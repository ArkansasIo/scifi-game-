// include/game/systems.h -- detailed ship systems and weapon control.
//
// The starship database (data/starships.h) describes what a hull *could*
// carry.  This module describes the ship the player is actually flying right
// now: which internal systems are installed and how healthy they are, how
// reactor power is routed between them, and what each weapon mount is aimed
// at and set to do.
//
//   systems      engineering: install, damage, repair, power routing
//   weapons      gunnery: load mounts, select target, pick fire mode, fire
//   control      the interactive panel that drives both ("buttons")
//
// Everything is deterministic and stateful in one place (tacticalSystems()),
// so a battle, a menu screen and the HUD all read the same truth.
//
// See docs/SYSTEMS.md.

#ifndef SBW_GAME_SYSTEMS_H
#define SBW_GAME_SYSTEMS_H

#include "core/globals.h"

/* ---------------- limits ---------------- */

#define MAX_SUBSYSTEMS 12
#define MAX_WEAPON_MOUNTS 8
#define MAX_TARGETS 6

/* ---------------- internal systems ---------------- */

// The subsystems every hull carries.  Each draws reactor power and, when
// damaged, degrades the ship in a specific, visible way.
enum shipsystemid
{
    SYSSYS_NONE = 0,
    SYSSYS_SENSORS,      // scan range, target lock quality
    SYSSYS_COMPUTER,     // fire control, weapon accuracy
    SYSSYS_PHASERS,      // energy weapon banks
    SYSSYS_WARP,         // navigation / retreat
    SYSSYS_TORPEDOES,    // projectile magazine
    SYSSYS_SHIELDS,      // shield regeneration
    SYSSYS_LIFE_SUPPORT, // crew, boarding defence
    SYSSYS_REACTOR,      // power output
    SYSSYS_COUNT
};

// How a system is doing.  Health maps onto these bands.
enum systemcondition
{
    SYSCOND_OFFLINE = 0, // 0%     -- destroyed, contributes nothing
    SYSCOND_CRITICAL,    // 1-25%  -- works badly, at risk of failure
    SYSCOND_DAMAGED,     // 26-60%
    SYSCOND_NOMINAL,     // 61-99%
    SYSCOND_OPTIMAL,     // 100%
    SYSCOND_COUNT
};

// One installed internal system.
struct shipsystem
{
    char name[24];
    shipsystemid id;
    int health;         // 0..100
    int maxHealth;      // always 100, kept for symmetry with hull bars
    int powerDraw;      // reactor units drawn at full operation
    int powerAllocated; // units the player has routed here
    int critical;       // 1 = loss of this system ends the mission
    char effect[48];    // what it governs, for the status screen
};

/* ---------------- weapon control ---------------- */

// How a mount is told to behave when it fires.
enum firemode
{
    FIRE_SINGLE = 0, // one shot, no bonus
    FIRE_BURST,      // two shots, +accuracy, +heat
    FIRE_WIDE,       // hits every target, -damage each
    FIRE_FOCUSED,    // +damage, -accuracy, best vs one hard target
    FIRE_DEFENSIVE,  // point-defence: intercepts incoming fire
    FIRE_HOLD,       // do not fire
    FIRE_MODE_COUNT
};

// A weapon mount: a weapon from weaponTable plus live state.
struct weaponmount
{
    char name[32];
    int weaponIndex;    // index into weaponTable, or -1 when empty
    int arc;            // 0 fore, 1 aft, 2 port, 3 starboard
    int health;         // 0..100, a hit can knock a mount out
    int ammo;           // -1 = unlimited (energy), else shots left
    int cooldown;       // turns until it can fire again
    int charges;        // shots loaded for the next fire order
    int powerAllocated; // reactor units routed to this mount
    firemode mode;
    int target; // index into the target list, -1 = none
    int armed;  // 1 = will fire on the next volley
};

/* ---------------- targets ---------------- */

struct tacticaltarget
{
    char name[30];
    int hull;
    int maxHull;
    int shields;
    int evasion; // 0..100
    int range;   // light-seconds
    int threat;  // 0..100, how hard it is hitting back
};

/* ---------------- power routing ---------------- */

struct powergrid
{
    int reactorOutput; // total units available this turn
    int allocated;     // units currently assigned
    int reserve;       // output - allocated
    int overloaded;    // 1 when allocated exceeds output
};

/* ---------------- aggregate ship state ---------------- */

struct tacticalsystems
{
    shipsystem systems[SYSSYS_COUNT];
    int systemCount;

    weaponmount mounts[MAX_WEAPON_MOUNTS];
    int mountCount;

    tacticaltarget targets[MAX_TARGETS];
    int targetCount;
    int selectedTarget;

    powergrid power;

    int hull;
    int maxHull;
    int shields;
    int maxShields;
    int crew;
    int maxCrew;
    int alert; // 0 green, 1 yellow, 2 red
    int heat;  // 0..100; high heat degrades accuracy
    int lastVolleyDamage;
    int volleysFired;
    int systemsDestroyed;
    char log[12][80];
    int logCount;
};

/* ---------------- lifecycle ---------------- */

void systemsInit();  // install the default systems and mounts
void systemsReset(); // back to a clean, fully repaired ship
tacticalsystems &tacticalSystems();
const tacticalsystems &tacticalSystemsConst();

/* ---------------- engineering: systems ---------------- */

// Recompute every system's powerAllocated from the current power grid and
// system health.  Call after any change to power or health.
void systemsRecomputePower();

// Condition band for a health value.
systemcondition systemCondition(int health);
const char *systemConditionName(systemcondition condition);
const char *systemName(shipsystemid id);

// Find a system by id.  Returns null if the ship does not carry it.
shipsystem *systemFind(shipsystemid id);
const shipsystem *systemFindConst(shipsystemid id);

// Apply `damage` to a system.  Returns the health actually lost.
int systemDamage(shipsystemid id, int damage);

// Repair a system by `amount`.  Returns the health actually restored.
int systemRepair(shipsystemid id, int amount);

// Repair everything a little (damage-control teams working each turn).
void systemRepairTick(int amount);

// The worst-off system, for the alert banner.  Returns null if all nominal.
const shipsystem *systemMostDamaged();

// How healthy the whole engineering plant is, 0..100.
int systemsIntegrity();

/* ---------------- engineering: power ---------------- */

// Total power the installed systems want at full health.
int powerDemand();
int powerReserve();

// Route `units` of reserve power into a system (or weapon mount by index).
// Returns the units actually moved.
int powerRouteToSystem(shipsystemid id, int units);
int powerRouteToMount(int mountIndex, int units);

// Free power back into the reserve.
int powerRecallFromSystem(shipsystemid id, int units);
int powerRecallFromMount(int mountIndex, int units);

// Hand out the reserve evenly across systems that are below demand.
void powerAutoBalance();

/* ---------------- gunnery: mounts ---------------- */

// Install a weapon from weaponTable into the first free mount.
// Returns the mount index, or -1 when there is no room / no such weapon.
int mountInstall(int weaponIndex, int arc);
int mountClear(int mountIndex);

weaponmount *mountAt(int index);
const weaponmount *mountAtConst(int index);

// Set a mount's fire mode / target / armed flag.  Returns 1 on success.
int mountSetMode(int mountIndex, firemode mode);
int mountSetTarget(int mountIndex, int targetIndex);
int mountArm(int mountIndex, int armed);

// A mount can fire when it is armed, has ammo, is off cooldown, is healthy
// and has power.  Returns 1 when ready.
int mountCanFire(int mountIndex);

// Every mount's readiness, for the panel: counts of ready / blocked.
int mountsReadyCount();
int mountsBlockedCount();

const char *fireModeName(firemode mode);
const char *arcName(int arc);

/* ---------------- gunnery: targets ---------------- */

int targetAdd(const char *name, int hull, int shields, int evasion, int range);
void targetClear();
int targetSetSelected(int index);
const tacticaltarget *targetSelected();

/* ---------------- resolution ---------------- */

// Load charges into every armed mount (bounded by ammo and power).
void loadWeapons();

// Fire the current volley at the selected target.  Returns total damage.
// Writes a human-readable report into the log.
int fireVolley();

// Enemy return fire; applies damage across hull, shields and systems.
int incomingFire(int strength);

// Advance one tactical turn: cool mounts, regen shields, tick heat,
// run damage control, refresh the alert level.
void tacticalTick();

/* ---------------- presentation ---------------- */

void showSystemStatus(); // engineering report, one row per system
void showPowerGrid();    // reactor output, allocation, reserve
void showWeaponStatus(); // one row per mount: mode, ammo, target
void showTargetList();   // the current target set
void showTacticalLog();  // the rolling volley log
void showControlPanel(); // the combined "buttons" panel

/* ---------------- interactive control panel ---------------- */

// The button-driven loop: digits pick an action, 0 returns.
// This is what the gameplay menu opens.
void runControlPanel();

#endif /* SBW_GAME_SYSTEMS_H */