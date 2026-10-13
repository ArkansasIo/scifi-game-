// src/game/systems.c++ -- detailed ship systems and weapon control.
//
// The engineering and gunnery layer.  It owns one tacticalsystems object
// (the ship the player is flying) and the operations that act on it:
//
//   engineering   install, damage, repair, route reactor power
//   gunnery       load mounts, pick a target and fire mode, fire a volley
//   control       an interactive, button-driven panel over both
//
// Nothing here reaches into the legacy battle path; it reads weaponTable
// (data/starships.c++) for weapon stats and gameplayProfile() for the ship's
// name, and is otherwise self-contained.  See docs/SYSTEMS.md.

#include "spacebattlerpg.h"
#include "game/systems.h"
#include "ui/display.h"

/* ---------------- storage ---------------- */

static tacticalsystems g_systems;
static int g_initialised = 0;

tacticalsystems &tacticalSystems()
{
    if (!g_initialised)
        systemsInit();

    return g_systems;
}

const tacticalsystems &tacticalSystemsConst()
{
    return tacticalSystems();
}

/* ---------------- internal helpers ---------------- */

static int clampInt(int value, int low, int high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static void sysLog(const char *text)
{
    tacticalsystems &ship = g_systems;

    if (ship.logCount < 12)
    {
        strncpy(ship.log[ship.logCount], text, 79);
        ship.log[ship.logCount][79] = '\0';
        ship.logCount++;
        return;
    }

    // Roll the log: drop the oldest line and append at the end.
    for (int i = 1; i < 12; ++i)
        strcpy(ship.log[i - 1], ship.log[i]);

    strncpy(ship.log[11], text, 79);
    ship.log[11][79] = '\0';
}

/* ---------------- engineering: systems ---------------- */

const char *systemName(shipsystemid id)
{
    switch (id)
    {
    case SYSSYS_SENSORS:
        return "Sensors";
    case SYSSYS_COMPUTER:
        return "Computer";
    case SYSSYS_PHASERS:
        return "Phaser banks";
    case SYSSYS_WARP:
        return "Warp drive";
    case SYSSYS_TORPEDOES:
        return "Torpedo tubes";
    case SYSSYS_SHIELDS:
        return "Shield grid";
    case SYSSYS_LIFE_SUPPORT:
        return "Life support";
    case SYSSYS_REACTOR:
        return "Reactor";
    default:
        return "Unknown";
    }
}

systemcondition systemCondition(int health)
{
    if (health <= 0)
        return SYSCOND_OFFLINE;
    if (health <= 25)
        return SYSCOND_CRITICAL;
    if (health <= 60)
        return SYSCOND_DAMAGED;
    if (health < 100)
        return SYSCOND_NOMINAL;
    return SYSCOND_OPTIMAL;
}

const char *systemConditionName(systemcondition condition)
{
    switch (condition)
    {
    case SYSCOND_OFFLINE:
        return "OFFLINE";
    case SYSCOND_CRITICAL:
        return "CRITICAL";
    case SYSCOND_DAMAGED:
        return "DAMAGED";
    case SYSCOND_NOMINAL:
        return "NOMINAL";
    case SYSCOND_OPTIMAL:
        return "OPTIMAL";
    default:
        return "UNKNOWN";
    }
}

shipsystem *systemFind(shipsystemid id)
{
    for (int i = 0; i < g_systems.systemCount; ++i)
    {
        if (g_systems.systems[i].id == id)
            return &g_systems.systems[i];
    }

    return 0;
}

const shipsystem *systemFindConst(shipsystemid id)
{
    return systemFind(id);
}

int systemDamage(shipsystemid id, int damage)
{
    shipsystem *sys = systemFind(id);

    if (!sys || damage <= 0)
        return 0;

    int before = sys->health;
    sys->health = clampInt(sys->health - damage, 0, sys->maxHealth);

    int lost = before - sys->health;

    if (lost > 0)
        systemsRecomputePower();

    return lost;
}

int systemRepair(shipsystemid id, int amount)
{
    shipsystem *sys = systemFind(id);

    if (!sys || amount <= 0)
        return 0;

    int before = sys->health;
    sys->health = clampInt(sys->health + amount, 0, sys->maxHealth);

    int gained = sys->health - before;

    if (gained > 0)
        systemsRecomputePower();

    return gained;
}

void systemRepairTick(int amount)
{
    if (amount <= 0)
        return;

    // Damage-control teams favour the systems that are closest to failing.
    for (int i = 0; i < g_systems.systemCount; ++i)
    {
        shipsystem &sys = g_systems.systems[i];

        if (sys.health < sys.maxHealth)
        {
            int repair = (sys.health < 25) ? amount * 2 : amount;
            sys.health = clampInt(sys.health + repair, 0, sys.maxHealth);
        }
    }

    systemsRecomputePower();
}

const shipsystem *systemMostDamaged()
{
    const shipsystem *worst = 0;

    for (int i = 0; i < g_systems.systemCount; ++i)
    {
        const shipsystem &sys = g_systems.systems[i];

        if (!worst || sys.health < worst->health)
            worst = &sys;
    }

    if (worst && worst->health >= worst->maxHealth)
        return 0;

    return worst;
}

int systemsIntegrity()
{
    if (g_systems.systemCount == 0)
        return 100;

    int total = 0;

    for (int i = 0; i < g_systems.systemCount; ++i)
        total += g_systems.systems[i].health;

    return total / g_systems.systemCount;
}

/* ---------------- engineering: power ---------------- */

int powerDemand()
{
    int demand = 0;

    for (int i = 0; i < g_systems.systemCount; ++i)
    {
        // A damaged system wants less power: it cannot use what it cannot
        // channel, so demand scales with health.
        const shipsystem &sys = g_systems.systems[i];
        demand += (sys.powerDraw * sys.health) / 100;
    }

    for (int i = 0; i < g_systems.mountCount; ++i)
    {
        const weaponmount &mount = g_systems.mounts[i];

        if (mount.weaponIndex < 0)
            continue;

        demand += (weaponAt(mount.weaponIndex)->powerDraw * mount.health) / 100;
    }

    return demand;
}

int powerReserve()
{
    int used = 0;

    for (int i = 0; i < g_systems.systemCount; ++i)
        used += g_systems.systems[i].powerAllocated;

    for (int i = 0; i < g_systems.mountCount; ++i)
        used += g_systems.mounts[i].powerAllocated;

    return g_systems.power.reactorOutput - used;
}

void systemsRecomputePower()
{
    int used = 0;

    for (int i = 0; i < g_systems.systemCount; ++i)
        used += g_systems.systems[i].powerAllocated;

    for (int i = 0; i < g_systems.mountCount; ++i)
        used += g_systems.mounts[i].powerAllocated;

    g_systems.power.allocated = used;
    g_systems.power.reserve = g_systems.power.reactorOutput - used;
    g_systems.power.overloaded = (used > g_systems.power.reactorOutput) ? 1 : 0;
}

int powerRouteToSystem(shipsystemid id, int units)
{
    shipsystem *sys = systemFind(id);

    if (!sys || units <= 0)
        return 0;

    int reserve = powerReserve();

    if (reserve <= 0)
        return 0;

    int want = (sys->powerDraw * sys->health) / 100;

    if (sys->powerAllocated >= want)
        return 0; // already fully powered

    int move = units;

    if (move > reserve)
        move = reserve;
    if (move > want - sys->powerAllocated)
        move = want - sys->powerAllocated;

    sys->powerAllocated += move;
    systemsRecomputePower();

    return move;
}

int powerRouteToMount(int mountIndex, int units)
{
    if (mountIndex < 0 || mountIndex >= g_systems.mountCount)
        return 0;

    weaponmount &mount = g_systems.mounts[mountIndex];

    if (mount.weaponIndex < 0 || units <= 0)
        return 0;

    int reserve = powerReserve();

    if (reserve <= 0)
        return 0;

    int want = (weaponAt(mount.weaponIndex)->powerDraw * mount.health) / 100;

    if (mount.powerAllocated >= want)
        return 0;

    int move = units;

    if (move > reserve)
        move = reserve;
    if (move > want - mount.powerAllocated)
        move = want - mount.powerAllocated;

    mount.powerAllocated += move;
    systemsRecomputePower();

    return move;
}

int powerRecallFromSystem(shipsystemid id, int units)
{
    shipsystem *sys = systemFind(id);

    if (!sys || units <= 0)
        return 0;

    int move = units;

    if (move > sys->powerAllocated)
        move = sys->powerAllocated;

    sys->powerAllocated -= move;
    systemsRecomputePower();

    return move;
}

int powerRecallFromMount(int mountIndex, int units)
{
    if (mountIndex < 0 || mountIndex >= g_systems.mountCount || units <= 0)
        return 0;

    weaponmount &mount = g_systems.mounts[mountIndex];
    int move = units;

    if (move > mount.powerAllocated)
        move = mount.powerAllocated;

    mount.powerAllocated -= move;
    systemsRecomputePower();

    return move;
}

void powerAutoBalance()
{
    // Hand the reserve to the systems that are furthest below demand, one
    // unit at a time, so a small reserve is spread evenly.
    int guard = 0;

    while (powerReserve() > 0 && guard++ < 512)
    {
        shipsystem *poorest = 0;
        int poorestGap = 0;

        for (int i = 0; i < g_systems.systemCount; ++i)
        {
            shipsystem &sys = g_systems.systems[i];
            int want = (sys.powerDraw * sys.health) / 100;
            int gap = want - sys.powerAllocated;

            if (gap > poorestGap)
            {
                poorestGap = gap;
                poorest = &sys;
            }
        }

        if (!poorest)
            break;

        poorest->powerAllocated += 1;
    }

    systemsRecomputePower();
}

/* ---------------- gunnery: mounts ---------------- */

const char *arcName(int arc)
{
    switch (arc)
    {
    case 0:
        return "fore";
    case 1:
        return "aft";
    case 2:
        return "port";
    case 3:
        return "starboard";
    default:
        return "omni";
    }
}

const char *fireModeName(firemode mode)
{
    switch (mode)
    {
    case FIRE_SINGLE:
        return "single";
    case FIRE_BURST:
        return "burst";
    case FIRE_WIDE:
        return "wide";
    case FIRE_FOCUSED:
        return "focused";
    case FIRE_DEFENSIVE:
        return "defensive";
    case FIRE_HOLD:
        return "hold";
    default:
        return "unknown";
    }
}

weaponmount *mountAt(int index)
{
    if (index < 0 || index >= g_systems.mountCount)
        return 0;

    return &g_systems.mounts[index];
}

const weaponmount *mountAtConst(int index)
{
    return mountAt(index);
}

int mountInstall(int weaponIndex, int arc)
{
    if (weaponIndex < 0 || weaponIndex >= weaponRecordCount)
        return -1;

    if (g_systems.mountCount >= MAX_WEAPON_MOUNTS)
        return -1;

    const weaponprofile *weapon = weaponAt(weaponIndex);

    if (!weapon)
        return -1;

    weaponmount &mount = g_systems.mounts[g_systems.mountCount];

    strncpy(mount.name, weapon->name, 31);
    mount.name[31] = '\0';
    mount.weaponIndex = weaponIndex;
    mount.arc = clampInt(arc, 0, 3);
    mount.health = 100;
    mount.ammo = (weapon->ammoCost > 0) ? 6 : -1; // magazine; energy is unlimited
    mount.cooldown = 0;
    mount.charges = 0;
    mount.powerAllocated = 0;
    mount.mode = FIRE_SINGLE;
    mount.target = -1;
    mount.armed = 0;

    int index = g_systems.mountCount;
    g_systems.mountCount++;

    systemsRecomputePower();

    return index;
}

int mountClear(int mountIndex)
{
    if (mountIndex < 0 || mountIndex >= g_systems.mountCount)
        return 0;

    // Shift the later mounts down so there are no holes.
    for (int i = mountIndex; i < g_systems.mountCount - 1; ++i)
        g_systems.mounts[i] = g_systems.mounts[i + 1];

    g_systems.mountCount--;

    systemsRecomputePower();

    return 1;
}

int mountSetMode(int mountIndex, firemode mode)
{
    weaponmount *mount = mountAt(mountIndex);

    if (!mount)
        return 0;

    if (mode < 0 || mode >= FIRE_MODE_COUNT)
        return 0;

    mount->mode = mode;

    return 1;
}

int mountSetTarget(int mountIndex, int targetIndex)
{
    weaponmount *mount = mountAt(mountIndex);

    if (!mount)
        return 0;

    if (targetIndex < -1 || targetIndex >= g_systems.targetCount)
        return 0;

    mount->target = targetIndex;

    return 1;
}

int mountArm(int mountIndex, int armed)
{
    weaponmount *mount = mountAt(mountIndex);

    if (!mount)
        return 0;

    mount->armed = armed ? 1 : 0;

    return 1;
}

int mountCanFire(int mountIndex)
{
    const weaponmount *mount = mountAtConst(mountIndex);

    if (!mount || mount->weaponIndex < 0)
        return 0;

    if (!mount->armed || mount->mode == FIRE_HOLD)
        return 0;
    if (mount->health <= 0 || mount->cooldown > 0)
        return 0;
    if (mount->ammo == 0)
        return 0;

    const weaponprofile *weapon = weaponAt(mount->weaponIndex);

    if (!weapon)
        return 0;

    // A mount needs at least the power a single shot draws.
    int need = weapon->powerDraw;

    if (mount->powerAllocated < need && mount->powerAllocated < 1)
        return 0;

    return 1;
}

int mountsReadyCount()
{
    int count = 0;

    for (int i = 0; i < g_systems.mountCount; ++i)
    {
        if (mountCanFire(i))
            count++;
    }

    return count;
}

int mountsBlockedCount()
{
    int count = 0;

    for (int i = 0; i < g_systems.mountCount; ++i)
    {
        if (!mountCanFire(i))
            count++;
    }

    return count;
}

/* ---------------- gunnery: targets ---------------- */

int targetAdd(const char *name, int hull, int shields, int evasion, int range)
{
    if (g_systems.targetCount >= MAX_TARGETS)
        return -1;

    tacticaltarget &target = g_systems.targets[g_systems.targetCount];

    strncpy(target.name, name ? name : "Contact", 29);
    target.name[29] = '\0';
    target.hull = hull;
    target.maxHull = hull > 0 ? hull : 1;
    target.shields = shields;
    target.evasion = clampInt(evasion, 0, 100);
    target.range = range;
    target.threat = 0;

    int index = g_systems.targetCount;
    g_systems.targetCount++;

    if (g_systems.selectedTarget < 0)
        g_systems.selectedTarget = index;

    return index;
}

void targetClear()
{
    g_systems.targetCount = 0;
    g_systems.selectedTarget = -1;

    for (int i = 0; i < g_systems.mountCount; ++i)
        g_systems.mounts[i].target = -1;
}

int targetSetSelected(int index)
{
    if (index < 0 || index >= g_systems.targetCount)
        return 0;

    g_systems.selectedTarget = index;

    return 1;
}

const tacticaltarget *targetSelected()
{
    if (g_systems.selectedTarget < 0 ||
        g_systems.selectedTarget >= g_systems.targetCount)
        return 0;

    return &g_systems.targets[g_systems.selectedTarget];
}

/* ---------------- resolution ---------------- */

void loadWeapons()
{
    for (int i = 0; i < g_systems.mountCount; ++i)
    {
        weaponmount &mount = g_systems.mounts[i];

        if (mount.weaponIndex < 0 || mount.health <= 0)
            continue;
        if (mount.cooldown > 0 || mount.ammo == 0)
            continue;

        const weaponprofile *weapon = weaponAt(mount.weaponIndex);

        if (!weapon)
            continue;

        switch (mount.mode)
        {
        case FIRE_BURST:
            mount.charges = 2;
            break;
        case FIRE_WIDE:
            mount.charges = 1;
            break;
        case FIRE_HOLD:
            mount.charges = 0;
            break;
        default:
            mount.charges = 1;
            break;
        }

        if (mount.ammo > 0 && mount.charges > mount.ammo)
            mount.charges = mount.ammo;
    }
}

// Effective accuracy of a mount against a target, in percent.
static int mountAccuracy(int mountIndex, const tacticaltarget *target)
{
    const weaponmount *mount = mountAtConst(mountIndex);

    if (!mount || mount->weaponIndex < 0)
        return 0;

    const weaponprofile *weapon = weaponAt(mount->weaponIndex);

    if (!weapon)
        return 0;

    int accuracy = weapon->accuracy;

    // Damaged computer and sensors cost accuracy.
    const shipsystem *computer = systemFindConst(SYSSYS_COMPUTER);
    const shipsystem *sensors = systemFindConst(SYSSYS_SENSORS);

    if (computer)
        accuracy += (computer->health - 100) / 4; // -25 at zero
    if (sensors)
        accuracy += (sensors->health - 100) / 8;

    // Mount condition and heat.
    accuracy += (mount->health - 100) / 5;
    accuracy -= g_systems.heat / 5;

    if (mount->mode == FIRE_BURST)
        accuracy += 5;
    else if (mount->mode == FIRE_FOCUSED)
        accuracy -= 10;

    if (target)
        accuracy -= target->evasion / 2;

    return clampInt(accuracy, 1, 99);
}

static int mountDamage(int mountIndex, const tacticaltarget *target)
{
    const weaponmount *mount = mountAtConst(mountIndex);

    if (!mount || mount->weaponIndex < 0)
        return 0;

    const weaponprofile *weapon = weaponAt(mount->weaponIndex);

    if (!weapon)
        return 0;

    int damage = weapon->damage;

    // Phaser banks and power feed the shot.
    const shipsystem *phasers = systemFindConst(SYSSYS_PHASERS);

    if (phasers)
        damage = (damage * phasers->health) / 100;

    damage += (mount->powerAllocated / 4);

    if (mount->mode == FIRE_FOCUSED)
        damage = (damage * 150) / 100;
    else if (mount->mode == FIRE_BURST)
        damage = (damage * 60) / 100; // each of two shots is lighter
    else if (mount->mode == FIRE_WIDE)
        damage = (damage * 70) / 100;

    if (target && target->shields > 0)
        damage = (damage * (100 + weapon->shieldBonus)) / 100;

    return damage > 0 ? damage : 1;
}

int fireVolley()
{
    loadWeapons();

    int total = 0;
    int hits = 0;
    int shots = 0;

    // The selected target is the fallback when a mount has none of its own.
    const tacticaltarget *primary = targetSelected();

    for (int i = 0; i < g_systems.mountCount; ++i)
    {
        weaponmount &mount = g_systems.mounts[i];

        if (!mountCanFire(i))
            continue;

        int shotsThis = mount.charges > 0 ? mount.charges : 1;
        int targetIndex = (mount.target >= 0) ? mount.target
                                              : g_systems.selectedTarget;

        const tacticaltarget *target =
            (targetIndex >= 0 && targetIndex < g_systems.targetCount)
                ? &g_systems.targets[targetIndex]
                : primary;

        for (int s = 0; s < shotsThis; ++s)
        {
            if (mount.ammo == 0)
                break;

            shots++;

            if (mount.ammo > 0)
                mount.ammo--;

            int accuracy = mountAccuracy(i, target);
            int roll = randomNumber(100);

            if (roll > accuracy)
                continue;

            int damage = mountDamage(i, target);

            if (target)
            {
                tacticaltarget *live =
                    (targetIndex >= 0 && targetIndex < g_systems.targetCount)
                        ? &g_systems.targets[targetIndex]
                        : &g_systems.targets[g_systems.selectedTarget];

                int absorbed = live->shields;

                if (absorbed > damage)
                    absorbed = damage;

                live->shields -= absorbed;
                damage -= absorbed;

                live->hull -= damage;

                if (live->hull < 0)
                    live->hull = 0;
            }

            total += damage;
            hits++;
        }

        // Fire modes that fire once still spend the turn.
        mount.cooldown = weaponAt(mount.weaponIndex)->cooldown;
        mount.charges = 0;
        mount.armed = 0;
    }

    // A full volley heats the ship.
    g_systems.heat = clampInt(g_systems.heat + shots * 3, 0, 100);
    g_systems.volleysFired++;
    g_systems.lastVolleyDamage = total;

    char line[80];

    if (shots == 0)
    {
        strcpy(line, "No mount was ready to fire.");
        sysLog(line);
        return 0;
    }

    snprintf(line, sizeof(line), "Volley: %d/%d shots hit for %d damage.",
             hits, shots, total);
    sysLog(line);

    return total;
}

int incomingFire(int strength)
{
    if (strength <= 0)
        return 0;

    int remaining = strength;

    // Shields absorb first, then hull, then a chance to knock out a system.
    if (g_systems.shields > 0)
    {
        int absorbed = g_systems.shields;

        if (absorbed > remaining)
            absorbed = remaining;

        g_systems.shields -= absorbed;
        remaining -= absorbed;
    }

    int hullLost = remaining;

    if (hullLost > g_systems.hull)
        hullLost = g_systems.hull;

    g_systems.hull -= hullLost;
    remaining -= hullLost;

    // Penetrating hits can damage a random system, weighted toward the ones
    // that are already weak.
    if (hullLost >= 10 && g_systems.systemCount > 0)
    {
        int index = randomNumber(g_systems.systemCount) - 1;

        if (index < 0)
            index = 0;
        if (index >= g_systems.systemCount)
            index = g_systems.systemCount - 1;

        int lost = systemDamage(g_systems.systems[index].id, hullLost / 2);

        if (lost > 0)
        {
            char line[80];
            snprintf(line, sizeof(line), "%s took %d damage.",
                     g_systems.systems[index].name, lost);
            sysLog(line);

            if (g_systems.systems[index].health <= 0)
                g_systems.systemsDestroyed++;
        }
    }

    char line[80];
    snprintf(line, sizeof(line), "Incoming fire: %d hull lost.", hullLost);
    sysLog(line);

    return hullLost;
}

void tacticalTick()
{
    // Cool mounts and the ship.
    for (int i = 0; i < g_systems.mountCount; ++i)
    {
        if (g_systems.mounts[i].cooldown > 0)
            g_systems.mounts[i].cooldown--;
    }

    if (g_systems.heat > 0)
        g_systems.heat -= 10;

    // Shield regeneration scales with the shield grid's health.
    const shipsystem *shields = systemFindConst(SYSSYS_SHIELDS);

    if (shields && shields->health > 0)
    {
        int regen = (g_systems.maxShields / 10) * shields->health / 100;

        if (regen < 1)
            regen = 1;

        g_systems.shields = clampInt(g_systems.shields + regen, 0,
                                     g_systems.maxShields);
    }

    // Damage control.
    systemRepairTick(2);

    // Alert level: red when a target is close or a critical system is hurt.
    const shipsystem *worst = systemMostDamaged();

    if ((worst && worst->health <= 25) || g_systems.hull * 2 < g_systems.maxHull)
        g_systems.alert = 2;
    else if ((worst && worst->health < 100) || g_systems.targetCount > 0)
        g_systems.alert = 1;
    else
        g_systems.alert = 0;
}

/* ---------------- lifecycle ---------------- */

static void installSystem(shipsystemid id, int powerDraw, int critical,
                          const char *effect)
{
    shipsystem &sys = g_systems.systems[g_systems.systemCount];

    strncpy(sys.name, systemName(id), 23);
    sys.name[23] = '\0';
    sys.id = id;
    sys.health = 100;
    sys.maxHealth = 100;
    sys.powerDraw = powerDraw;
    sys.powerAllocated = 0;
    sys.critical = critical;
    strncpy(sys.effect, effect ? effect : "", 47);
    sys.effect[47] = '\0';

    g_systems.systemCount++;
}

void systemsInit()
{
    memset(&g_systems, 0, sizeof(g_systems));

    g_systems.systemCount = 0;

    installSystem(SYSSYS_SENSORS, 5, 0, "scan range and target lock");
    installSystem(SYSSYS_COMPUTER, 8, 0, "fire control accuracy");
    installSystem(SYSSYS_PHASERS, 20, 0, "energy weapon damage");
    installSystem(SYSSYS_WARP, 15, 0, "navigation and retreat");
    installSystem(SYSSYS_TORPEDOES, 6, 0, "projectile magazine");
    installSystem(SYSSYS_SHIELDS, 18, 0, "shield regeneration");
    installSystem(SYSSYS_LIFE_SUPPORT, 10, 1, "crew survival");
    installSystem(SYSSYS_REACTOR, 0, 1, "power generation");

    g_systems.mountCount = 0;
    g_systems.targetCount = 0;
    g_systems.selectedTarget = -1;

    g_systems.power.reactorOutput = 120;
    g_systems.power.allocated = 0;
    g_systems.power.reserve = g_systems.power.reactorOutput;
    g_systems.power.overloaded = 0;

    g_systems.hull = 100;
    g_systems.maxHull = 100;
    g_systems.shields = 50;
    g_systems.maxShields = 50;
    g_systems.crew = 40;
    g_systems.maxCrew = 40;
    g_systems.alert = 0;
    g_systems.heat = 0;
    g_systems.lastVolleyDamage = 0;
    g_systems.volleysFired = 0;
    g_systems.systemsDestroyed = 0;
    g_systems.logCount = 0;

    // The ship is named after the player's vessel when one exists.
    strncpy(g_systems.systems[0].name, systemName(SYSSYS_SENSORS), 23);

    // Install a default loadout: a beam, a torpedo and a point-defence gun,
    // chosen from the real weapon table when it is populated.
    if (weaponRecordCount > 0)
    {
        int beam = findWeapon("Phaser Array");
        int torpedo = findWeapon("Photon Torpedo");
        int pd = findWeapon("Point Defence");

        if (beam < 0)
            beam = 0;
        if (torpedo < 0)
            torpedo = (weaponRecordCount > 1) ? 1 : 0;
        if (pd < 0)
            pd = (weaponRecordCount > 2) ? 2 : 0;

        mountInstall(beam, 0);
        mountInstall(torpedo, 0);
        mountInstall(pd, 1);
    }

    // Route power sensibly, then start fully ready.
    powerAutoBalance();

    g_initialised = 1;

    sysLog("Engineering online. All systems nominal.");
}

void systemsReset()
{
    g_initialised = 0;
    systemsInit();
}

/* ---------------- presentation ---------------- */

void showSystemStatus()
{
    const tacticalsystems &ship = tacticalSystemsConst();

    drawHeader("Ship Systems");

    drawField("Integrity", systemsIntegrity());
    drawField("Systems", ship.systemCount);

    const shipsystem *worst = systemMostDamaged();

    if (worst)
        drawFieldText("Attention", worst->name);
    else
        drawFieldText("Attention", "none - all nominal");

    cout << endl;

    for (int i = 0; i < ship.systemCount; ++i)
    {
        const shipsystem &sys = ship.systems[i];

        cout << "  " << (i + 1) << ") " << sys.name;

        int len = (int)strlen(sys.name);
        for (int pad = len; pad < 16; ++pad)
            cout << ' ';

        cout << "  ";

        // A compact health bar, then the condition word.
        int filled = sys.health / 10;

        cout << "[";
        for (int b = 0; b < 10; ++b)
            cout << (b < filled ? '#' : '-');
        cout << "] ";

        cout << systemConditionName(systemCondition(sys.health));
        cout << "  pwr " << sys.powerAllocated << "/"
             << (sys.powerDraw * sys.health) / 100;

        if (sys.critical)
            cout << "  (critical)";

        cout << endl;
    }

    cout << endl;
}

void showPowerGrid()
{
    const tacticalsystems &ship = tacticalSystemsConst();

    drawHeader("Power Grid");

    drawField("Reactor output", ship.power.reactorOutput);
    drawField("Allocated", ship.power.allocated);
    drawField("Reserve", ship.power.reserve);
    drawFieldText("Status", ship.power.overloaded ? "OVERLOADED" : "stable");
    drawField("Demand", powerDemand());

    cout << endl;

    drawBar("Load", ship.power.allocated, ship.power.reactorOutput, 30);

    cout << endl;
}

void showWeaponStatus()
{
    const tacticalsystems &ship = tacticalSystemsConst();

    drawHeader("Weapons");

    if (ship.mountCount == 0)
    {
        out("No weapon mounts fitted.");
        cout << endl;
        return;
    }

    drawField("Ready", mountsReadyCount());
    drawField("Blocked", mountsBlockedCount());

    cout << endl;

    for (int i = 0; i < ship.mountCount; ++i)
    {
        const weaponmount &mount = ship.mounts[i];

        cout << "  " << (i + 1) << ") " << mount.name
             << " [" << arcName(mount.arc) << "]" << endl;

        cout << "      mode " << fireModeName(mount.mode)
             << "  health " << mount.health
             << "  ammo " << (mount.ammo < 0 ? -1 : mount.ammo)
             << "  power " << mount.powerAllocated
             << "  cooldown " << mount.cooldown
             << "  " << (mount.armed ? "ARMED" : "safe") << endl;

        if (mount.target >= 0 && mount.target < ship.targetCount)
            cout << "      target: " << ship.targets[mount.target].name << endl;
    }

    cout << endl;
}

void showTargetList()
{
    const tacticalsystems &ship = tacticalSystemsConst();

    drawHeader("Targets");

    if (ship.targetCount == 0)
    {
        out("No contacts on sensors.");
        cout << endl;
        return;
    }

    for (int i = 0; i < ship.targetCount; ++i)
    {
        const tacticaltarget &target = ship.targets[i];

        cout << "  " << (i + 1) << ") " << target.name;

        if (i == ship.selectedTarget)
            cout << "  <-- selected";

        cout << endl;

        cout << "      hull " << target.hull << "/" << target.maxHull
             << "  shields " << target.shields
             << "  evasion " << target.evasion
             << "  range " << target.range << " ls"
             << "  threat " << target.threat << endl;
    }

    cout << endl;
}

void showTacticalLog()
{
    const tacticalsystems &ship = tacticalSystemsConst();

    drawHeader("Tactical Log");

    if (ship.logCount == 0)
    {
        out("(no entries)");
        cout << endl;
        return;
    }

    for (int i = 0; i < ship.logCount; ++i)
        out(ship.log[i]);

    cout << endl;
}

void showControlPanel()
{
    const tacticalsystems &ship = tacticalSystemsConst();

    drawHeader("Tactical Control");

    // Status strip.
    const char *alertText = (ship.alert == 2)   ? "RED ALERT"
                            : (ship.alert == 1) ? "YELLOW ALERT"
                                                : "GREEN";

    cout << "  " << alertText
         << "   integrity " << systemsIntegrity() << "%"
         << "   ready " << mountsReadyCount() << "/" << ship.mountCount
         << "   heat " << ship.heat << "%" << endl;
    cout << endl;

    drawBar("Hull   ", ship.hull, ship.maxHull, 24);
    drawBar("Shields", ship.shields, ship.maxShields, 24);
    drawBar("Power  ", ship.power.allocated, ship.power.reactorOutput, 24);
    drawBar("Crew   ", ship.crew, ship.maxCrew, 24);

    cout << endl;

    const tacticaltarget *target = targetSelected();

    if (target)
    {
        cout << "  Target: " << target->name
             << "  hull " << target->hull << "/" << target->maxHull
             << "  shields " << target->shields << endl;
    }
    else
    {
        cout << "  Target: none selected" << endl;
    }

    cout << endl;
}

/* ---------------- interactive control panel ---------------- */

static void panelFireVolley()
{
    clearScreen();
    drawHeader("Fire");

    int damage = fireVolley();

    drawField("Damage dealt", damage);
    drawField("Mounts ready", mountsReadyCount());
    drawField("Heat", tacticalSystems().heat);

    cout << endl;

    const tacticaltarget *target = targetSelected();

    if (target)
    {
        cout << "  " << target->name << " now at hull " << target->hull;

        if (target->hull <= 0)
            cout << "  *** DESTROYED ***";

        cout << endl;
    }

    cout << endl;
}

static void panelLoadAll()
{
    clearScreen();
    drawHeader("Load Weapons");

    loadWeapons();

    const tacticalsystems &ship = tacticalSystemsConst();

    for (int i = 0; i < ship.mountCount; ++i)
    {
        const weaponmount &mount = ship.mounts[i];

        cout << "  " << mount.name
             << "  loaded " << mount.charges
             << "  ammo " << (mount.ammo < 0 ? -1 : mount.ammo) << endl;
    }

    cout << endl;
}

static void panelRoutePower()
{
    clearScreen();
    drawHeader("Route Power");

    const tacticalsystems &ship = tacticalSystemsConst();

    drawField("Reserve", powerReserve());
    drawField("Demand", powerDemand());

    cout << endl;

    for (int i = 0; i < ship.systemCount; ++i)
    {
        const shipsystem &sys = ship.systems[i];

        cout << "  " << (i + 1) << ") " << sys.name
             << "  allocated " << sys.powerAllocated
             << "/" << (sys.powerDraw * sys.health) / 100 << endl;
    }

    cout << "  0) Back   A) Auto-balance" << endl
         << endl;

    char answer = readChar("System (number, A, or 0): ");

    if (answer == '0')
        return;

    if (answer == 'A')
    {
        powerAutoBalance();
        out("Reserve distributed across the plant.");
        return;
    }

    int index = answer - '1';

    if (index < 0 || index >= ship.systemCount)
    {
        out("No such system.");
        return;
    }

    int units = askInt("Units to route: ", 0, 120);

    if (units <= 0)
        return;

    int moved = powerRouteToSystem(ship.systems[index].id, units);

    cout << endl;

    if (moved > 0)
    {
        char line[80];
        snprintf(line, sizeof(line), "Routed %d power to %s.",
                 moved, ship.systems[index].name);
        out(line);
    }
    else
    {
        out("No power moved (no reserve, or the system is fully powered).");
    }

    cout << endl;
}

static void panelSetFireMode()
{
    clearScreen();
    drawHeader("Fire Mode");

    const tacticalsystems &ship = tacticalSystemsConst();

    if (ship.mountCount == 0)
    {
        out("No weapon mounts fitted.");
        cout << endl;
        return;
    }

    for (int i = 0; i < ship.mountCount; ++i)
    {
        cout << "  " << (i + 1) << ") " << ship.mounts[i].name
             << "  mode " << fireModeName(ship.mounts[i].mode) << endl;
    }

    cout << "  0) Back" << endl
         << endl;

    int which = askInt("Mount: ", 0, ship.mountCount);

    if (which <= 0)
        return;

    clearScreen();
    drawHeader("Choose Mode");

    for (int m = 0; m < FIRE_MODE_COUNT; ++m)
        cout << "  " << (m + 1) << ") " << fireModeName((firemode)m) << endl;

    cout << "  0) Back" << endl
         << endl;

    int mode = askInt("Mode: ", 0, FIRE_MODE_COUNT);

    if (mode <= 0)
        return;

    if (mountSetMode(which - 1, (firemode)(mode - 1)))
        out("Fire mode set.");
    else
        out("Could not set the fire mode.");
}

static void panelSetTarget()
{
    clearScreen();
    drawHeader("Select Target");

    const tacticalsystems &ship = tacticalSystemsConst();

    if (ship.targetCount == 0)
    {
        out("No contacts on sensors.");
        cout << endl;
        return;
    }

    showTargetList();

    int which = askInt("Target (0 to cancel): ", 0, ship.targetCount);

    if (which <= 0)
        return;

    targetSetSelected(which - 1);
    out("Target selected.");
}

static void panelToggleArm()
{
    clearScreen();
    drawHeader("Arm / Safe");

    const tacticalsystems &ship = tacticalSystemsConst();

    if (ship.mountCount == 0)
    {
        out("No weapon mounts fitted.");
        cout << endl;
        return;
    }

    for (int i = 0; i < ship.mountCount; ++i)
    {
        cout << "  " << (i + 1) << ") " << ship.mounts[i].name
             << "  " << (ship.mounts[i].armed ? "ARMED" : "safe") << endl;
    }

    cout << "  0) Back" << endl
         << endl;

    int which = askInt("Mount: ", 0, ship.mountCount);

    if (which <= 0)
        return;

    weaponmount *mount = mountAt(which - 1);

    if (!mount)
        return;

    mountArm(which - 1, !mount->armed);

    out(mount->armed ? "Mount armed." : "Mount set to safe.");
}

static void panelRepair()
{
    clearScreen();
    drawHeader("Damage Control");

    const tacticalsystems &ship = tacticalSystemsConst();

    for (int i = 0; i < ship.systemCount; ++i)
    {
        const shipsystem &sys = ship.systems[i];

        cout << "  " << (i + 1) << ") " << sys.name
             << "  health " << sys.health << "%" << endl;
    }

    cout << "  0) Back" << endl
         << endl;

    int which = askInt("Repair which system: ", 0, ship.systemCount);

    if (which <= 0)
        return;

    int amount = askInt("Repair amount (crew effort): ", 1, 50);

    int restored = systemRepair(ship.systems[which - 1].id, amount);

    cout << endl;

    if (restored > 0)
    {
        char line[80];
        snprintf(line, sizeof(line), "%s repaired by %d.",
                 ship.systems[which - 1].name, restored);
        out(line);
    }
    else
    {
        out("Nothing to repair.");
    }

    cout << endl;
}

void runControlPanel()
{
    static const char *entries[] = {
        "Full status report",
        "Engineering - ship systems",
        "Engineering - power grid",
        "Engineering - route power",
        "Engineering - damage control",
        "Gunnery - weapon status",
        "Gunnery - set fire mode",
        "Gunnery - select target",
        "Gunnery - arm / safe mounts",
        "Gunnery - load weapons",
        "Gunnery - fire volley",
        "Targets - list contacts",
        "Advance one tactical turn",
        "Tactical log"};

    static void (*handlers[])() = {
        showControlPanel,
        showSystemStatus,
        showPowerGrid,
        panelRoutePower,
        panelRepair,
        showWeaponStatus,
        panelSetFireMode,
        panelSetTarget,
        panelToggleArm,
        panelLoadAll,
        panelFireVolley,
        showTargetList,
        tacticalTick,
        showTacticalLog};

    runSubMenu("Tactical Control", entries, handlers, 14);
}