# Ship Systems and Weapon Control

The engineering and gunnery layer of **spacebattlerpg 2.0.0**. Where
[`STARSHIPS.md`](STARSHIPS.md) describes what a hull *could* carry, this
document describes the ship the player is actually flying right now: which
internal systems are installed, how healthy they are, how reactor power is
routed between them, and what each weapon mount is aimed at and set to do.

Implemented in `include/game/systems.h` and `src/game/systems.c++`.

---

## 1. Model

One `tacticalsystems` object holds the whole live ship. It is created by
`systemsInit()` and read everywhere through `tacticalSystems()`:

```
tacticalsystems
├── systems[SYSSYS_COUNT]    internal systems and their health / power
├── mounts[MAX_WEAPON_MOUNTS] weapon mounts and their live state
├── targets[MAX_TARGETS]      sensor contacts
├── power                     reactor output, allocation, reserve
└── hull / shields / crew / heat / alert / log
```

Nothing here touches the legacy battle path. The only external reads are
`weaponTable` (from `data/starships.c++`) for weapon stats and
`gameplayProfile()` for ship naming.

---

## 2. Internal systems

Every hull carries the same eight systems. Each draws reactor power and, when
damaged, degrades the ship in a specific, visible way.

| System | Power | Critical | Governs |
| --- | --- | --- | --- |
| Sensors | 5 | no | scan range and target lock |
| Computer | 8 | no | fire-control accuracy |
| Phaser banks | 20 | no | energy weapon damage |
| Warp drive | 15 | no | navigation and retreat |
| Torpedo tubes | 6 | no | projectile magazine |
| Shield grid | 18 | no | shield regeneration |
| Life support | 10 | **yes** | crew survival |
| Reactor | 0 | **yes** | power generation |

Health runs 0–100 and maps onto a condition band:

| Band | Health | Meaning |
| --- | --- | --- |
| `SYSCOND_OFFLINE` | 0% | destroyed, contributes nothing |
| `SYSCOND_CRITICAL` | 1–25% | works badly, at risk of failure |
| `SYSCOND_DAMAGED` | 26–60% | degraded |
| `SYSCOND_NOMINAL` | 61–99% | below peak |
| `SYSCOND_OPTIMAL` | 100% | full |

### Damage and repair

```
systemDamage(id, damage)     returns the health actually lost
systemRepair(id, amount)     returns the health actually restored
systemRepairTick(amount)     damage-control teams, every turn
systemMostDamaged()          the worst system, for the alert banner
systemsIntegrity()           whole-plant health, 0..100
```

Damage-control teams favour the systems closest to failing: a system below
25% is repaired at double rate. Both `systemDamage` and `systemRepair`
recompute the power grid automatically, because a damaged system can no
longer draw its full allocation.

---

## 3. Power routing

The reactor produces a fixed output (`reactorOutput`, 120 by default). Every
system and every mount wants power; a system's demand scales with its health,
so a half-wrecked phaser bank wants half its normal draw.

```
powerDemand()                    total demand at current health
powerReserve()                   output - allocated
powerRouteToSystem(id, units)    move reserve into a system
powerRouteToMount(index, units)  move reserve into a weapon
powerRecallFromSystem(id, units) free power back
powerAutoBalance()               spread the reserve across the plant
```

`power.reserve` may never go negative through the routing API; `power.overloaded`
is set if allocation ever exceeds output. `powerAutoBalance()` hands the
reserve out one unit at a time to whichever system is furthest below demand,
so a small reserve is spread evenly rather than dumped in one place.

---

## 4. Weapon control

### Mounts

A mount is a weapon from `weaponTable` plus live state:

| Field | Meaning |
| --- | --- |
| `weaponIndex` | index into `weaponTable`, or `-1` when empty |
| `arc` | 0 fore, 1 aft, 2 port, 3 starboard |
| `health` | 0–100; a penetrating hit can knock a mount out |
| `ammo` | `-1` unlimited (energy), else shots remaining |
| `cooldown` | turns until it can fire again |
| `charges` | shots loaded for the next volley |
| `powerAllocated` | reactor units routed to this mount |
| `mode` | fire mode (below) |
| `target` | index into the target list, `-1` = use the selected target |
| `armed` | 1 = will fire on the next volley |

`mountCanFire()` is the single readiness test: armed, not on hold, healthy,
off cooldown, has ammo and has power. `mountsReadyCount()` /
`mountsBlockedCount()` summarise it for the panel.

### Fire modes

| Mode | Effect |
| --- | --- |
| `FIRE_SINGLE` | one shot, no bonus |
| `FIRE_BURST` | two lighter shots, **+5 accuracy** |
| `FIRE_WIDE` | hits every target, 70% damage each |
| `FIRE_FOCUSED` | **+50% damage**, **−10 accuracy**, best vs one hard target |
| `FIRE_DEFENSIVE` | point-defence, intercepts incoming fire |
| `FIRE_HOLD` | does not fire |

### Accuracy

```
accuracy = weapon.accuracy
         + (computer.health - 100) / 4      // -25 at zero
         + (sensors.health  - 100) / 8
         + (mount.health    - 100) / 5
         - heat / 5
         + mode bonus (burst +5, focused -10)
         - target.evasion / 2
```

Clamped to 1–99%. A damaged computer is the single largest accuracy penalty,
which is why it is worth repairing first.

### Damage

```
damage = weapon.damage * phasers.health / 100     // phaser feed
       + powerAllocated / 4                       // extra power
       * mode multiplier (focused 150%, burst 60%, wide 70%)
       * (100 + weapon.shieldBonus) / 100         // vs a shielded target
```

Shields absorb the shot first, then hull. A full volley adds heat
(`3` per shot fired, capped at 100); `tacticalTick()` bleeds 10 heat per turn.

### Firing a volley

```
loadWeapons()   load charges into every armed mount (bounded by ammo/power)
fireVolley()    roll accuracy per shot, apply damage, spend ammo, set cooldown
incomingFire(strength)  enemy return fire: shields, hull, then a system hit
tacticalTick()  cool mounts, regen shields, tick heat, run damage control
```

`fireVolley()` returns the total damage dealt and appends a report line to the
rolling log. `incomingFire()` subtracts shields first, then hull, and a
penetrating hit of 10+ has a chance to damage a random system (weighted toward
the ones already weak).

### Alert level

`tacticalTick()` recomputes the alert each turn:

* **RED** — a system at ≤25%, or hull below half
* **YELLOW** — any system below 100%, or contacts on sensors
* **GREEN** — everything nominal

---

## 5. The control panel ("buttons")

`runControlPanel()` is a numbered, button-driven loop over both engineering
and gunnery. It is opened from the **Empire** menu (**V**) as *Tactical
control*.

| Button | Action |
| --- | --- |
| 1 | Full status report (hull, shields, power, crew, target) |
| 2 | Engineering — ship systems |
| 3 | Engineering — power grid |
| 4 | Engineering — route power |
| 5 | Engineering — damage control |
| 6 | Gunnery — weapon status |
| 7 | Gunnery — set fire mode |
| 8 | Gunnery — select target |
| 9 | Gunnery — arm / safe mounts |
| 10 | Gunnery — load weapons |
| 11 | Gunnery — fire volley |
| 12 | Targets — list contacts |
| 13 | Advance one tactical turn |
| 14 | Tactical log |

As everywhere in the game: digits pick, `0` returns.

---

## 6. Wiring

The module is pulled in by the master header and compiled by `build.ps1`:

```cpp
#include "spacebattlerpg.h"   // -> include/game/systems.h
```

```
src/game/systems.c++   // listed in build.ps1 alongside menus.c++ / gamelogic.c++
```

Menu entry points live in `game/menus.c++`:

| Function | Screen |
| --- | --- |
| `menuShipSystems()` | `showSystemStatus()` |
| `menuPowerGrid()` | `showPowerGrid()` + `showWeaponStatus()` |
| `menuWeapons()` | `showWeaponStatus()` + `showTargetList()` |
| `menuTacticalControl()` | `runControlPanel()` |

---

## 7. Open Questions

* Weapon mounts are installed from a fixed default loadout in `systemsInit()`;
  fitting from the fleet / drydock screens is not yet wired.
* `incomingFire()` picks a system to damage at random. A hit-location table
  keyed on the firing arc would be more tactical.
* Fire modes are per-mount but a volley resolves all mounts at once; a
  per-mount fire order queue would allow staggered fire.