// src/data/starships.c++ -- the known starship database.
//
// Hull classes from fighter to flagship, the weapons they mount, and the
// components that fit into their slots.  See docs/STARSHIPS.md.

#include "spacebattlerpg.h"
#include "data/starships.h"

/* ---------------- storage ---------------- */

shipclassdef shipClassTable[MAX_SHIPCLASSES];
component componentTable[MAX_COMPONENTS];
weaponprofile weaponTable[MAX_WEAPONS];

int shipClassCount = 0;
int componentCount = 0;
int weaponRecordCount = 0;

/* ---------------- labels ---------------- */

static const char *hullNames[HULL_COUNT] = {
    "Fighter", "Interceptor", "Bomber", "Corvette", "Frigate",
    "Destroyer", "Cruiser", "Battleship", "Carrier", "Dreadnought",
    "Titan", "Flagship", "Station"};

static const char *weaponTypeNames[WTYPE_COUNT] = {
    "None", "Laser", "Plasma", "Railgun", "Missile", "Torpedo",
    "Ion Cannon", "Particle Beam", "Flak", "Point Defence", "Tesla",
    "Graviton",
    "Phaser", "Disruptor", "Photon Torpedo", "Quantum Torpedo",
    "Polaron Beam", "Plasma Torpedo", "Transphasic Torpedo",
    "Tricobalt Device", "Chroniton Torpedo", "Antiproton Beam",
    "Thalaron Weapon", "Borg Cutting Beam", "Breen Energy Dampener",
    "Voth Antiproton Cannon"};

static const char *componentKindNames[COMP_COUNT] = {
    "None", "Weapon", "Shield", "Armor", "Engine", "Reactor",
    "Sensor", "Targeting", "Point Defence", "Fighter Bay", "Repair",
    "Cloak", "AI Core"};

static const char *shipTypeNames[SHIP_COUNT] = {
    "Armored", "Shielded", "Hybrid", "Stealth"};

const char *hullClassName(hullclass hull)
{
    if (hull < 0 || hull >= HULL_COUNT)
        return "Unknown";

    return hullNames[hull];
}

const char *weaponTypeName(weapontype type)
{
    if (type < 0 || type >= WTYPE_COUNT)
        return "Unknown";

    return weaponTypeNames[type];
}

const char *componentKindName(componentkind kind)
{
    if (kind < 0 || kind >= COMP_COUNT)
        return "Unknown";

    return componentKindNames[kind];
}

const char *shipTypeName(shiptype type)
{
    if (type < 0 || type >= SHIP_COUNT)
        return "Unknown";

    return shipTypeNames[type];
}

const char *hullRoleName(hullclass hull)
{
    switch (hull)
    {
    case HULL_FIGHTER:
    case HULL_INTERCEPTOR:
    case HULL_BOMBER:
        return "screen";
    case HULL_CORVETTE:
    case HULL_FRIGATE:
        return "escort";
    case HULL_DESTROYER:
    case HULL_CRUISER:
        return "line";
    case HULL_BATTLESHIP:
    case HULL_CARRIER:
    case HULL_DREADNOUGHT:
        return "capital";
    case HULL_TITAN:
    case HULL_FLAGSHIP:
        return "super-capital";
    case HULL_STATION:
        return "installation";
    default:
        return "unknown";
    }
}

/* ---------------- hull table ---------------- */

// name, hull, type, tier, cost, hullHP, shieldHP, armor, evasion, speed,
// maneuver, reactor, crewReq, crewCap, supply, maintenance,
// wSlots, dSlots, sSlots, fighters, role, buildTurns, description
static const shipclassdef hullSeeds[] = {
    {"Wasp Interceptor", HULL_FIGHTER, SHIP_SHIELDED, 1, 120,
     80, 120, 4, 85, 180, 90, 60, 1, 1, 1, 1, 1, 1, 0, 0,
     "screen", 1, "Fast, fragile, and cheap. Screens the line."},

    {"Raptor Interceptor", HULL_INTERCEPTOR, SHIP_SHIELDED, 2, 260,
     140, 220, 6, 80, 200, 95, 110, 2, 2, 2, 2, 2, 1, 1, 0,
     "screen", 2, "Anti-fighter specialist with high evasion."},

    {"Hammer Bomber", HULL_BOMBER, SHIP_ARMORED, 2, 320,
     220, 80, 14, 45, 120, 55, 140, 3, 3, 3, 3, 2, 1, 1, 0,
     "screen", 2, "Carries torpedoes. Deadly against capitals."},

    {"Sabre Corvette", HULL_CORVETTE, SHIP_HYBRID, 2, 420,
     320, 260, 12, 65, 150, 70, 200, 6, 8, 4, 4, 3, 2, 1, 0,
     "escort", 3, "Balanced patrol craft. Common pirate hull."},

    {"Bulwark Frigate", HULL_FRIGATE, SHIP_ARMORED, 3, 780,
     620, 380, 22, 50, 120, 50, 340, 14, 18, 7, 6, 4, 3, 2, 0,
     "escort", 4, "Anti-fighter escort with point-defence arrays."},

    {"Lancer Destroyer", HULL_DESTROYER, SHIP_HYBRID, 3, 1250,
     980, 720, 30, 40, 105, 42, 620, 30, 40, 11, 10, 6, 4, 3, 0,
     "line", 6, "Front-line gun platform. Spine-mounted railgun."},

    {"Vigil Cruiser", HULL_CRUISER, SHIP_SHIELDED, 4, 2400,
     1800, 1700, 40, 32, 92, 34, 1200, 70, 95, 18, 18, 8, 6, 4, 1,
     "line", 9, "Independent operator. Long endurance, deep magazine."},

    {"Sovereign Battleship", HULL_BATTLESHIP, SHIP_HYBRID, 5, 5200,
     4200, 3600, 62, 22, 78, 26, 2600, 180, 240, 34, 36, 12, 9, 6, 2,
     "capital", 14, "The line breaker. Broadside of particle beams."},

    {"Aegis Carrier", HULL_CARRIER, SHIP_SHIELDED, 5, 5800,
     3600, 4200, 48, 18, 70, 22, 2800, 160, 420, 30, 40, 8, 10, 8, 24,
     "capital", 16, "Projects fighter wings across the battlespace."},

    {"Obliterator Dreadnought", HULL_DREADNOUGHT, SHIP_ARMORED, 6, 11000,
     8800, 6400, 90, 12, 62, 18, 5200, 420, 560, 60, 80, 18, 14, 10, 4,
     "capital", 24, "Siege hull. Built to crack stations and worlds."},

    {"Colossus Titan", HULL_TITAN, SHIP_ARMORED, 7, 24000,
     18000, 12000, 120, 8, 50, 12, 9000, 900, 1100, 95, 160, 24, 20, 14, 8,
     "super-capital", 40, "A fleet in one hull. Rarely seen, never forgotten."},

    {"Divine Throne Ship", HULL_FLAGSHIP, SHIP_HYBRID, 8, 48000,
     34000, 26000, 140, 10, 55, 14, 16000, 1600, 2000, 150, 280, 30, 24, 18, 12,
     "super-capital", 60, "The Divine Order's command vessel. Raid target."},

    {"Orbital Fortress", HULL_STATION, SHIP_ARMORED, 4, 3600,
     7200, 2400, 70, 0, 0, 0, 1800, 120, 300, 20, 30, 14, 10, 8, 6,
     "installation", 20, "Static defence. Cannot move, very hard to kill."},

    /* ---------------- Star Trek ships ---------------- */

    /* Federation Starfleet */
    {"NX-class Enterprise", HULL_FRIGATE, SHIP_ARMORED, 2, 900,
     700, 500, 20, 48, 130, 55, 420, 18, 22, 8, 7, 4, 3, 2, 0,
     "escort", 5, "Enterprise NX-01. Warp 5 pioneer of the 22nd century."},
    {"Constitution-class", HULL_CRUISER, SHIP_SHIELDED, 3, 2200,
     1600, 1500, 36, 34, 95, 36, 1100, 60, 90, 16, 16, 8, 5, 4, 0,
     "line", 9, "Enterprise NCC-1701. The classic five-year mission."},
    {"Miranda-class", HULL_FRIGATE, SHIP_SHIELDED, 3, 1100,
     850, 780, 24, 44, 115, 50, 560, 24, 30, 9, 8, 4, 3, 2, 0,
     "escort", 5, "Reliant. Workhorse escort of the TOS film era."},
    {"Excelsior-class", HULL_BATTLESHIP, SHIP_HYBRID, 4, 3400,
     2600, 2200, 48, 28, 88, 30, 1700, 95, 130, 24, 10, 7, 5, 0, 1,
     "capital", 12, "Transwarp testbed turned long-serving explorer."},
    {"Miranda-class Refit", HULL_DESTROYER, SHIP_SHIELDED, 3, 1400,
     1050, 900, 28, 42, 110, 46, 680, 30, 38, 11, 10, 6, 4, 3, 0,
     "line", 7, "Upgraded Miranda with modern phasers and torpedoes."},
    {"Ambassador-class", HULL_BATTLESHIP, SHIP_SHIELDED, 5, 4600,
     3400, 3200, 54, 24, 84, 28, 2300, 150, 210, 30, 32, 12, 9, 6, 1,
     "capital", 14, "Enterprise-C. Heavy cruiser of the 2340s."},
    {"Galaxy-class", HULL_BATTLESHIP, SHIP_HYBRID, 6, 6800,
     5200, 4600, 68, 20, 80, 24, 3400, 240, 320, 44, 48, 14, 11, 8, 2,
     "capital", 18, "Enterprise-D. Flagship explorer of the 2360s."},
    {"Nebula-class", HULL_CARRIER, SHIP_SHIELDED, 6, 6400,
     4600, 5000, 60, 20, 76, 24, 3200, 200, 300, 38, 44, 12, 10, 9, 16,
     "capital", 17, "Galaxy saucer with a mission-specific pod."},
    {"Sovereign-class", HULL_DREADNOUGHT, SHIP_HYBRID, 7, 12000,
     9200, 8400, 88, 16, 70, 20, 5800, 420, 560, 66, 84, 18, 14, 10, 3,
     "capital", 26, "Enterprise-E. Fast, heavily armed battleship."},
    {"Intrepid-class", HULL_CRUISER, SHIP_SHIELDED, 6, 4200,
     3000, 3400, 44, 30, 96, 40, 2100, 140, 180, 26, 10, 7, 5, 1, 1,
     "line", 13, "Voyager. Long-range explorer with variable geometry."},
    {"Defiant-class", HULL_DESTROYER, SHIP_ARMORED, 6, 3600,
     2800, 2200, 62, 34, 100, 44, 1900, 60, 90, 22, 24, 10, 8, 6, 0,
     "line", 11, "Escort. Pulse phasers and quantum torpedoes."},
    {"Akira-class", HULL_CARRIER, SHIP_HYBRID, 6, 5600,
     4200, 3800, 58, 24, 88, 30, 2800, 180, 260, 34, 38, 12, 9, 7, 12,
     "capital", 15, "Torpedo-heavy carrier with fighter wings."},
    {"Prometheus-class", HULL_DREADNOUGHT, SHIP_HYBRID, 7, 11000,
     8600, 7800, 82, 18, 92, 26, 5400, 320, 420, 58, 74, 16, 12, 9, 2,
     "capital", 24, "Multi-vector assault mode. Splits into three."},
    {"Saber-class", HULL_FRIGATE, SHIP_SHIELDED, 5, 1600,
     1200, 1100, 30, 42, 120, 48, 820, 36, 44, 12, 11, 6, 4, 3, 0,
     "escort", 7, "Compact light escort, fast and agile."},
    {"Steamrunner-class", HULL_DESTROYER, SHIP_HYBRID, 6, 3000,
     2300, 1800, 46, 32, 100, 42, 1600, 90, 120, 18, 20, 8, 6, 5, 0,
     "line", 10, "Fast attack ship built during the Dominion War."},

    /* Klingon Empire */
    {"Klingon Bird-of-Prey", HULL_FRIGATE, SHIP_HYBRID, 4, 1800,
     1400, 1200, 34, 40, 110, 48, 900, 40, 60, 14, 14, 7, 5, 4, 0,
     "escort", 8, "Winged raider with a cloaking device."},
    {"K't'inga-class", HULL_CRUISER, SHIP_ARMORED, 5, 3000,
     2400, 1800, 50, 26, 86, 32, 1500, 90, 130, 22, 22, 10, 7, 5, 0,
     "line", 11, "Heavy Klingon battlecruiser. Disruptors and torpedoes."},
    {"Vor'cha-class", HULL_BATTLESHIP, SHIP_HYBRID, 6, 5400,
     4200, 3400, 64, 22, 82, 28, 2900, 180, 240, 34, 38, 13, 10, 7, 1,
     "capital", 16, "Klingon attack cruiser of the 2360s."},
    {"Negh'Var-class", HULL_DREADNOUGHT, SHIP_ARMORED, 7, 10000,
     8000, 6000, 96, 14, 66, 20, 5000, 380, 500, 58, 72, 17, 13, 9, 2,
     "capital", 22, "Gowron's flagship. Massive Klingon warship."},

    /* Romulan Star Empire */
    {"Romulan Warbird", HULL_DREADNOUGHT, SHIP_SHIELDED, 7, 10500,
     7800, 7600, 80, 16, 72, 22, 5200, 400, 520, 60, 78, 16, 13, 10, 2,
     "capital", 24, "D'deridex-class. Huge, cloaked, plasma-armed."},
    {"Romulan Scout", HULL_CORVETTE, SHIP_STEALTH, 3, 800,
     600, 500, 16, 60, 140, 60, 400, 12, 16, 6, 5, 3, 3, 2, 0,
     "screen", 4, "Small cloaked reconnaissance craft."},
    {"Romulan Drone", HULL_INTERCEPTOR, SHIP_STEALTH, 4, 1000,
     750, 600, 20, 66, 150, 66, 500, 0, 0, 7, 6, 4, 3, 3, 0,
     "screen", 5, "Automated cloaked attack drone."},

    /* Cardassian Union */
    {"Cardassian Galor", HULL_CRUISER, SHIP_HYBRID, 5, 2600,
     2000, 1600, 44, 28, 84, 30, 1300, 80, 110, 18, 9, 6, 5, 0, 0,
     "line", 10, "Galor-class. Mainstay Cardassian warship."},
    {"Cardassian Keldon", HULL_BATTLESHIP, SHIP_HYBRID, 6, 4400,
     3400, 2800, 56, 24, 78, 26, 2400, 150, 200, 30, 34, 12, 9, 7, 0,
     "capital", 15, "Keldon-class. Upgraded Galor with heavier guns."},

    /* Dominion / Borg / other powers */
    {"Jem'Hadar Fighter", HULL_INTERCEPTOR, SHIP_SHIELDED, 6, 1400,
     1000, 900, 22, 64, 150, 62, 620, 1, 1, 8, 7, 5, 4, 3, 0,
     "screen", 5, "Jem'Hadar attack ship. Polaron beams."},
    {"Jem'Hadar Battlecruiser", HULL_BATTLESHIP, SHIP_HYBRID, 7, 6200,
     5000, 4200, 72, 20, 76, 26, 3200, 200, 280, 38, 42, 14, 11, 8, 1,
     "capital", 17, "Dominion battlecruiser. Fast and hard-hitting."},
    {"Borg Sphere", HULL_TITAN, SHIP_HYBRID, 8, 26000,
     20000, 18000, 130, 12, 60, 18, 12000, 9000, 9000, 90, 140, 22, 18, 14, 0,
     "super-capital", 40, "Borg sphere. Adaptive shields, cutting beams."},
    {"Borg Cube", HULL_FLAGSHIP, SHIP_HYBRID, 9, 60000,
     48000, 42000, 160, 8, 50, 12, 24000, 130000, 130000, 140, 220, 26, 22, 18, 0,
     "super-capital", 60, "Borg cube. Assimilation, adaptation, inevitability."},
    {"Breen Warship", HULL_CRUISER, SHIP_HYBRID, 6, 3200,
     2500, 2000, 50, 28, 86, 32, 1600, 100, 140, 20, 22, 10, 7, 6, 0,
     "line", 11, "Breen cruiser with energy-dampening weapon."},
    {"Tholian Webspinner", HULL_FRIGATE, SHIP_ARMORED, 5, 1700,
     1300, 900, 40, 34, 100, 40, 800, 40, 60, 12, 13, 7, 5, 4, 0,
     "escort", 8, "Tholian vessel that spins webs of energy."},
    {"Species 8472 Bioship", HULL_DREADNOUGHT, SHIP_HYBRID, 8, 14000,
     11000, 9000, 100, 20, 82, 26, 6400, 1, 1, 70, 90, 18, 14, 10, 0,
     "capital", 28, "Organic bioship from fluidic space."},
    {"Ferengi Marauder", HULL_DESTROYER, SHIP_SHIELDED, 5, 2400,
     1800, 1600, 34, 30, 92, 36, 1200, 60, 80, 16, 16, 8, 6, 4, 0,
     "line", 9, "D'Kora-class. Armed trade vessel."},
    {"Vulcan D'Kyr", HULL_CRUISER, SHIP_SHIELDED, 4, 2000,
     1500, 1400, 32, 34, 94, 38, 1000, 50, 70, 14, 15, 7, 5, 4, 0,
     "line", 8, "Vulcan combat cruiser with ring-shaped hull."},
    {"Kazon Predator", HULL_FRIGATE, SHIP_ARMORED, 4, 1300,
     1000, 700, 30, 38, 104, 44, 660, 30, 40, 11, 11, 6, 4, 3, 0,
     "escort", 6, "Kazon raider. Crude but numerous."},
    {"Hirogen Hunter", HULL_DESTROYER, SHIP_HYBRID, 6, 2800,
     2200, 1700, 44, 34, 100, 42, 1500, 80, 110, 18, 20, 9, 6, 5, 0,
     "line", 10, "Hirogen warship. Built for the hunt."},
    {"Krenim Timeship", HULL_TITAN, SHIP_HYBRID, 8, 22000,
     16000, 15000, 110, 16, 68, 22, 10000, 700, 900, 80, 110, 20, 16, 12, 0,
     "super-capital", 36, "Krenim weapon ship. Erases targets from history."},
    {"Xindi Weapon", HULL_TITAN, SHIP_ARMORED, 8, 20000,
     15000, 8000, 130, 6, 40, 10, 8000, 500, 700, 70, 100, 18, 14, 10, 0,
     "super-capital", 34, "Xindi planet-killer. The probe of the Delphic Expanse."},
    {"Doomsday Machine", HULL_FLAGSHIP, SHIP_ARMORED, 9, 45000,
     36000, 20000, 200, 4, 30, 8, 14000, 1, 1, 120, 180, 24, 20, 16, 0,
     "super-capital", 50, "Planet-eating robot of unknown origin."}};

static const int hullSeedCount =
    (int)(sizeof(hullSeeds) / sizeof(hullSeeds[0]));

/* ---------------- weapon table ---------------- */

// name, type, damage, accuracy, range, cooldown, shieldBonus, armorBonus,
// ammo, power
static const weaponprofile weaponSeeds[] = {
    {"Pulse Laser", WTYPE_LASER, 40, 82, 3, 2, 90, 110, 0, 6},
    {"Heavy Laser", WTYPE_LASER, 90, 76, 4, 3, 95, 120, 0, 14},
    {"Plasma Lance", WTYPE_PLASMA, 160, 68, 5, 4, 120, 90, 0, 26},
    {"Fusion Cannon", WTYPE_PLASMA, 300, 60, 6, 6, 130, 85, 0, 48},
    {"Light Railgun", WTYPE_RAILGUN, 120, 74, 7, 4, 70, 150, 2, 22},
    {"Spinal Railgun", WTYPE_RAILGUN, 420, 62, 9, 8, 60, 180, 4, 60},
    {"Hornet Missile", WTYPE_MISSILE, 70, 88, 6, 3, 100, 120, 3, 12},
    {"Swarm Missiles", WTYPE_MISSILE, 55, 92, 5, 2, 105, 115, 2, 10},
    {"Heavy Torpedo", WTYPE_TORPEDO, 480, 66, 8, 9, 80, 200, 6, 70},
    {"Ion Cannon", WTYPE_ION, 110, 78, 5, 3, 200, 40, 0, 24},
    {"Ion Disruptor", WTYPE_ION, 220, 70, 6, 5, 240, 30, 0, 44},
    {"Particle Beam", WTYPE_PARTICLE_BEAM, 260, 80, 7, 5, 140, 140, 0, 52},
    {"Beam Lance", WTYPE_PARTICLE_BEAM, 520, 70, 10, 9, 150, 130, 0, 95},
    {"Flak Battery", WTYPE_FLAK, 45, 86, 4, 2, 70, 80, 1, 8},
    {"Point Defence Grid", WTYPE_POINT_DEFENCE, 30, 94, 2, 1, 60, 60, 1, 6},
    {"Tesla Arc", WTYPE_TESLA, 95, 84, 5, 3, 160, 70, 0, 20},
    {"Graviton Driver", WTYPE_GRAVITON, 380, 58, 9, 8, 110, 170, 0, 88},
    {"Singularity Projector", WTYPE_GRAVITON, 700, 50, 11, 12, 120, 190, 0, 140},

    /* ---------------- Star Trek weapons ---------------- */

    /* Federation phasers */
    {"Phase Pistol", WTYPE_PHASER, 25, 92, 2, 1, 130, 100, 0, 3},
    {"Phase Rifle", WTYPE_PHASER, 45, 90, 3, 2, 135, 105, 0, 5},
    {"Type VI Phaser Array", WTYPE_PHASER, 120, 88, 4, 2, 150, 110, 0, 12},
    {"Type X Phaser Array", WTYPE_PHASER, 260, 86, 6, 3, 160, 115, 0, 28},
    {"Type XII Phaser Array", WTYPE_PHASER, 520, 84, 8, 5, 170, 120, 0, 62},
    {"Type XIV Phaser Array", WTYPE_PHASER, 900, 80, 10, 7, 180, 125, 0, 120},
    {"Phaser Cannon", WTYPE_PHASER, 380, 82, 5, 4, 155, 130, 0, 48},
    {"Pulse Phaser Cannon", WTYPE_PHASER, 620, 78, 6, 5, 165, 135, 0, 90},
    {"Defiant Pulse Phaser", WTYPE_PHASER, 780, 80, 5, 5, 175, 140, 0, 110},

    /* Klingon / Romulan / Cardassian disruptors */
    {"Klingon Disruptor", WTYPE_DISRUPTOR, 200, 80, 5, 3, 100, 150, 0, 24},
    {"Heavy Disruptor Cannon", WTYPE_DISRUPTOR, 440, 72, 7, 5, 95, 165, 0, 58},
    {"Romulan Disruptor Array", WTYPE_DISRUPTOR, 560, 76, 8, 6, 105, 155, 0, 74},
    {"Cardassian Spiral Disruptor", WTYPE_DISRUPTOR, 340, 78, 6, 4, 110, 145, 0, 40},
    {"Breen Disruptor", WTYPE_DISRUPTOR, 300, 82, 6, 4, 120, 140, 0, 36},

    /* Torpedoes */
    {"Photon Torpedo", WTYPE_PHOTON_TORPEDO, 500, 68, 8, 8, 90, 210, 6, 66},
    {"Photon Torpedo Spread", WTYPE_PHOTON_TORPEDO, 320, 74, 6, 6, 95, 200, 4, 52},
    {"Quantum Torpedo", WTYPE_QUANTUM_TORPEDO, 820, 66, 9, 10, 100, 240, 8, 100},
    {"Quantum Torpedo Spread", WTYPE_QUANTUM_TORPEDO, 540, 72, 7, 8, 105, 230, 6, 82},
    {"Transphasic Torpedo", WTYPE_TRANSPHASIC, 1400, 60, 10, 14, 150, 260, 10, 160},
    {"Chroniton Torpedo", WTYPE_CHRONITON, 950, 62, 9, 12, 120, 220, 9, 130},
    {"Tricobalt Device", WTYPE_TRICOBALT, 1800, 48, 11, 18, 110, 300, 12, 210},

    /* Plasma torpedoes */
    {"Romulan Plasma Torpedo", WTYPE_PLASMA_TORPEDO, 460, 70, 8, 7, 130, 180, 5, 70},
    {"Gorn Plasma Torpedo", WTYPE_PLASMA_TORPEDO, 380, 68, 7, 6, 140, 170, 4, 56},

    /* Dominion / Gamma Quadrant */
    {"Polaron Beam", WTYPE_POLARON, 300, 82, 6, 4, 210, 90, 0, 42},
    {"Heavy Polaron Cannon", WTYPE_POLARON, 640, 74, 8, 6, 230, 80, 0, 96},
    {"Jem'Hadar Polaron Beam", WTYPE_POLARON, 460, 78, 7, 5, 220, 85, 0, 68},

    /* Antiproton */
    {"Antiproton Beam", WTYPE_ANTIPROTON, 540, 80, 8, 6, 190, 150, 0, 88},
    {"Voth Antiproton Cannon", WTYPE_VOTH, 880, 72, 9, 8, 200, 160, 0, 140},

    /* Exotic / special */
    {"Thalaron Weapon", WTYPE_THALERON, 2200, 40, 10, 20, 260, 260, 0, 260},
    {"Borg Cutting Beam", WTYPE_BORG_CUTTING, 1000, 90, 6, 6, 240, 220, 0, 150},
    {"Breen Energy Dampener", WTYPE_BREEN_ENERGY, 260, 88, 8, 6, 320, 40, 0, 70},
    {"Isolytic Burst", WTYPE_GRAVITON, 1100, 56, 10, 12, 150, 200, 0, 170}};

static const int weaponSeedCount =
    (int)(sizeof(weaponSeeds) / sizeof(weaponSeeds[0]));

/* ---------------- component table ---------------- */

// name, kind, tier, cost, mass, hp, powerDraw, powerOut, shield, armor,
// speed, sensor, accuracy, repair, damage type
static const component componentSeeds[] = {
    /* reactors */
    {"Fission Core I", COMP_REACTOR, 1, 120, 40, 40, 0, 200, 0, 0, 0, 0, 0, 0, DMG_TRUE},
    {"Fusion Core II", COMP_REACTOR, 2, 320, 70, 70, 0, 520, 0, 0, 0, 0, 0, 0, DMG_TRUE},
    {"Antimatter Core III", COMP_REACTOR, 3, 780, 110, 120, 0, 1200, 0, 0, 0, 0, 0, 0, DMG_TRUE},
    {"Quantum Core IV", COMP_REACTOR, 4, 1800, 160, 180, 0, 2800, 0, 0, 0, 0, 0, 0, DMG_TRUE},

    /* shields */
    {"Deflector I", COMP_SHIELD, 1, 100, 30, 30, 20, 0, 200, 0, 0, 0, 0, 0, DMG_TRUE},
    {"Deflector II", COMP_SHIELD, 2, 280, 55, 60, 45, 0, 560, 0, 0, 0, 0, 0, DMG_TRUE},
    {"Aegis Shield III", COMP_SHIELD, 3, 700, 90, 100, 90, 0, 1400, 0, 0, 0, 0, 0, DMG_TRUE},
    {"Aegis Shield IV", COMP_SHIELD, 4, 1600, 140, 160, 170, 0, 3200, 0, 0, 0, 0, 0, DMG_TRUE},

    /* armor */
    {"Ablative Plate I", COMP_ARMOR, 1, 90, 60, 120, 0, 0, 0, 60, 0, 0, 0, 0, DMG_TRUE},
    {"Ablative Plate II", COMP_ARMOR, 2, 240, 100, 260, 0, 0, 0, 150, 0, 0, 0, 0, DMG_TRUE},
    {"Crystal Weave III", COMP_ARMOR, 3, 620, 150, 480, 0, 0, 0, 320, 0, 0, 0, 0, DMG_TRUE},
    {"Neutronium Plate IV", COMP_ARMOR, 4, 1500, 220, 900, 0, 0, 0, 700, 0, 0, 0, 0, DMG_TRUE},

    /* engines */
    {"Ion Drive I", COMP_ENGINE, 1, 80, 50, 50, 40, 0, 0, 0, 40, 0, 0, 0, DMG_TRUE},
    {"Ion Drive II", COMP_ENGINE, 2, 220, 80, 90, 80, 0, 0, 0, 95, 0, 0, 0, DMG_TRUE},
    {"Warp Drive III", COMP_ENGINE, 3, 560, 120, 150, 150, 0, 0, 0, 180, 0, 0, 0, DMG_TRUE},

    /* sensors */
    {"Sensor Array I", COMP_SENSOR, 1, 70, 20, 30, 15, 0, 0, 0, 0, 60, 0, 0, DMG_TRUE},
    {"Deep Scanner II", COMP_SENSOR, 2, 200, 35, 50, 35, 0, 0, 0, 0, 150, 0, 0, DMG_TRUE},
    {"Quantum Sensor III", COMP_SENSOR, 3, 520, 55, 80, 70, 0, 0, 0, 0, 320, 0, 0, DMG_TRUE},

    /* targeting */
    {"Targeting Computer I", COMP_TARGETING, 1, 90, 15, 25, 20, 0, 0, 0, 0, 0, 40, 0, DMG_TRUE},
    {"Targeting Computer II", COMP_TARGETING, 2, 260, 25, 45, 45, 0, 0, 0, 0, 0, 95, 0, DMG_TRUE},
    {"Fire Control III", COMP_TARGETING, 3, 640, 40, 70, 90, 0, 0, 0, 0, 0, 200, 0, DMG_TRUE},

    /* point defence */
    {"PD Turret I", COMP_POINT_DEFENCE, 1, 110, 25, 40, 25, 0, 0, 0, 0, 40, 30, 0, DMG_PHYSICAL},
    {"PD Grid II", COMP_POINT_DEFENCE, 2, 300, 45, 70, 55, 0, 0, 0, 0, 90, 70, 0, DMG_PHYSICAL},

    /* fighter bays */
    {"Fighter Bay I", COMP_FIGHTER_BAY, 2, 400, 120, 150, 60, 0, 0, 0, 0, 30, 0, 0, DMG_TRUE},
    {"Fighter Bay II", COMP_FIGHTER_BAY, 3, 950, 190, 260, 120, 0, 0, 0, 0, 60, 0, 0, DMG_TRUE},

    /* repair */
    {"Repair Drone Bay I", COMP_REPAIR, 2, 340, 70, 90, 60, 0, 0, 40, 0, 0, 0, 60, DMG_TRUE},
    {"Repair Drone Bay II", COMP_REPAIR, 3, 820, 110, 150, 120, 0, 0, 90, 0, 0, 0, 150, DMG_TRUE},

    /* cloak */
    {"Stealth Field II", COMP_CLOAK, 2, 600, 80, 80, 120, 0, 0, 20, 0, 0, 0, 0, DMG_TRUE},

    /* ai core */
    {"AI Core I", COMP_AI_CORE, 2, 480, 40, 60, 70, 0, 0, 0, 0, 50, 50, 0, DMG_TRUE},
    {"AI Core II", COMP_AI_CORE, 3, 1200, 70, 110, 140, 0, 0, 0, 0, 120, 120, 0, DMG_TRUE},

    /* weapons as components */
    {"Pulse Laser Turret", COMP_WEAPON, 1, 150, 30, 40, 6, 0, 0, 0, 0, 0, 0, 0, DMG_PHYSICAL},
    {"Heavy Laser Turret", COMP_WEAPON, 2, 380, 60, 75, 14, 0, 0, 0, 0, 0, 0, 0, DMG_PHYSICAL},
    {"Plasma Lance Turret", COMP_WEAPON, 3, 900, 95, 120, 26, 0, 0, 0, 0, 0, 0, 0, DMG_FIRE},
    {"Railgun Battery", COMP_WEAPON, 2, 340, 70, 90, 22, 0, 0, 0, 0, 0, 0, 0, DMG_PHYSICAL},
    {"Spinal Railgun", COMP_WEAPON, 4, 1600, 180, 220, 60, 0, 0, 0, 0, 0, 0, 0, DMG_PHYSICAL},
    {"Missile Rack", COMP_WEAPON, 2, 300, 55, 70, 12, 0, 0, 0, 0, 0, 0, 0, DMG_PHYSICAL},
    {"Torpedo Tube", COMP_WEAPON, 3, 720, 100, 110, 70, 0, 0, 0, 0, 0, 0, 0, DMG_PHYSICAL},
    {"Ion Cannon Mount", COMP_WEAPON, 3, 780, 90, 130, 24, 0, 0, 0, 0, 0, 0, 0, DMG_LIGHTNING},
    {"Particle Beam Emitter", COMP_WEAPON, 4, 1450, 140, 180, 52, 0, 0, 0, 0, 0, 0, 0, DMG_ARCANE},
    {"Graviton Driver", COMP_WEAPON, 5, 2600, 220, 280, 88, 0, 0, 0, 0, 0, 0, 0, DMG_VOID}};

static const int componentSeedCount =
    (int)(sizeof(componentSeeds) / sizeof(componentSeeds[0]));

/* ---------------- initializers ---------------- */

void initializeStarships()
{
    shipClassCount = 0;

    for (int i = 0; i < hullSeedCount && i < MAX_SHIPCLASSES; ++i)
    {
        shipClassTable[shipClassCount] = hullSeeds[i];
        shipClassCount++;
    }
}

void initializeWeapons()
{
    weaponRecordCount = 0;

    for (int i = 0; i < weaponSeedCount && i < MAX_WEAPONS; ++i)
    {
        weaponTable[weaponRecordCount] = weaponSeeds[i];
        weaponRecordCount++;
    }
}

void initializeComponents()
{
    componentCount = 0;

    for (int i = 0; i < componentSeedCount && i < MAX_COMPONENTS; ++i)
    {
        componentTable[componentCount] = componentSeeds[i];
        componentCount++;
    }
}

/* ---------------- lookups ---------------- */

int findShipClass(const char *name)
{
    if (!name)
        return -1;

    for (int i = 0; i < shipClassCount; ++i)
    {
        if (strcmp(shipClassTable[i].name, name) == 0)
            return i;
    }

    return -1;
}

int findComponent(const char *name)
{
    if (!name)
        return -1;

    for (int i = 0; i < componentCount; ++i)
    {
        if (strcmp(componentTable[i].name, name) == 0)
            return i;
    }

    return -1;
}

int findWeapon(const char *name)
{
    if (!name)
        return -1;

    for (int i = 0; i < weaponRecordCount; ++i)
    {
        if (strcmp(weaponTable[i].name, name) == 0)
            return i;
    }

    return -1;
}

const shipclassdef *shipClassAt(int index)
{
    if (index < 0 || index >= shipClassCount)
        return &shipClassTable[0];

    return &shipClassTable[index];
}

const component *componentAt(int index)
{
    if (index < 0 || index >= componentCount)
        return 0;

    return &componentTable[index];
}

const weaponprofile *weaponAt(int index)
{
    if (index < 0 || index >= weaponRecordCount)
        return 0;

    return &weaponTable[index];
}

int shipsOfHull(hullclass hull, int out[], int outMax)
{
    if (!out || outMax <= 0)
        return 0;

    int written = 0;

    for (int i = 0; i < shipClassCount && written < outMax; ++i)
    {
        if (shipClassTable[i].hull == hull)
        {
            out[written] = i;
            written++;
        }
    }

    return written;
}

int componentsOfKind(componentkind kind, int out[], int outMax)
{
    if (!out || outMax <= 0)
        return 0;

    int written = 0;

    for (int i = 0; i < componentCount && written < outMax; ++i)
    {
        if (componentTable[i].kind == kind)
        {
            out[written] = i;
            written++;
        }
    }

    return written;
}

/* ---------------- fit maths ---------------- */

int fitCost(int shipClassIndex, const int componentIndices[], int count)
{
    const shipclassdef *hull = shipClassAt(shipClassIndex);

    if (shipClassIndex < 0 || shipClassIndex >= shipClassCount)
        return 0;

    int cost = hull->cost;

    for (int i = 0; i < count; ++i)
    {
        const component *c = componentAt(componentIndices[i]);

        if (c)
            cost += c->cost;
    }

    return cost;
}

int fitPowerBalance(int shipClassIndex, const int componentIndices[], int count)
{
    const shipclassdef *hull = shipClassAt(shipClassIndex);

    if (shipClassIndex < 0 || shipClassIndex >= shipClassCount)
        return 0;

    int balance = hull->reactorOutput;

    for (int i = 0; i < count; ++i)
    {
        const component *c = componentAt(componentIndices[i]);

        if (!c)
            continue;

        balance += c->powerOutput;
        balance -= c->powerDraw;
    }

    return balance;
}

int fittedShipValue(int shipClassIndex, const int componentIndices[], int count)
{
    const shipclassdef *hull = shipClassAt(shipClassIndex);

    if (shipClassIndex < 0 || shipClassIndex >= shipClassCount)
        return 0;

    // Offence: hull base, plus weapons.
    int value = hull->hullHP / 4;
    value += hull->shieldHP / 5;
    value += hull->armorRating * 3;
    value += hull->weaponSlots * 40;

    // Defence and systems from the fitted components.
    for (int i = 0; i < count; ++i)
    {
        const component *c = componentAt(componentIndices[i]);

        if (!c)
            continue;

        value += c->hp / 2;
        value += c->shieldValue / 3;
        value += c->armorValue / 3;
        value += c->sensorValue / 4;
        value += c->accuracyValue / 4;
        value += c->repairValue / 3;
        value += c->powerOutput / 6;
    }

    return value;
}

int validateFit(int shipClassIndex, const int componentIndices[], int count)
{
    const shipclassdef *hull = shipClassAt(shipClassIndex);

    if (shipClassIndex < 0 || shipClassIndex >= shipClassCount)
        return 1;

    int usedWeapon = 0;
    int usedDefence = 0;
    int usedSystem = 0;

    for (int i = 0; i < count; ++i)
    {
        const component *c = componentAt(componentIndices[i]);

        if (!c)
            return 2; // invalid component index

        switch (c->kind)
        {
        case COMP_WEAPON:
        case COMP_POINT_DEFENCE:
            usedWeapon++;
            break;

        case COMP_SHIELD:
        case COMP_ARMOR:
        case COMP_ENGINE:
            usedDefence++;
            break;

        default:
            usedSystem++;
            break;
        }
    }

    if (usedWeapon > hull->weaponSlots)
        return 3; // too many weapons

    if (usedDefence > hull->defenceSlots)
        return 4; // too many defences

    if (usedSystem > hull->systemSlots)
        return 5; // too many systems

    if (fitPowerBalance(shipClassIndex, componentIndices, count) < 0)
        return 6; // reactor overloaded

    return 0;
}

int weaponDamageVs(const weaponprofile &weapon, shiptype target)
{
    int damage = weapon.damage;

    switch (target)
    {
    case SHIP_ARMORED:
        // Armor is weak to energy, strong against kinetic.
        if (weapon.type == WTYPE_LASER || weapon.type == WTYPE_PLASMA || weapon.type == WTYPE_ION || weapon.type == WTYPE_PARTICLE_BEAM)
            damage = (damage * 120) / 100;
        else if (weapon.type == WTYPE_RAILGUN)
            damage = (damage * 80) / 100;
        break;

    case SHIP_SHIELDED:
        // Shields are weak to ion, strong against energy.
        if (weapon.type == WTYPE_ION)
            damage = (damage * 200) / 100;
        else if (weapon.type == WTYPE_LASER || weapon.type == WTYPE_PLASMA)
            damage = (damage * 85) / 100;
        else if (weapon.type == WTYPE_RAILGUN)
            damage = (damage * 130) / 100;
        break;

    case SHIP_HYBRID:
        break;

    case SHIP_STEALTH:
        // Stealth hulls are hard to hit, not hard to hurt.
        damage = (damage * 110) / 100;
        break;

    default:
        break;
    }

    return damage;
}

int bestWeaponForHull(hullclass hull)
{
    // Capital hulls want heavy weapons; screen hulls want fast ones.
    const char *preferred = "Pulse Laser";

    switch (hull)
    {
    case HULL_FIGHTER:
    case HULL_INTERCEPTOR:
        preferred = "Pulse Laser";
        break;
    case HULL_BOMBER:
        preferred = "Heavy Torpedo";
        break;
    case HULL_CORVETTE:
    case HULL_FRIGATE:
        preferred = "Hornet Missile";
        break;
    case HULL_DESTROYER:
        preferred = "Light Railgun";
        break;
    case HULL_CRUISER:
        preferred = "Heavy Laser";
        break;
    case HULL_BATTLESHIP:
        preferred = "Particle Beam";
        break;
    case HULL_CARRIER:
        preferred = "Flak Battery";
        break;
    case HULL_DREADNOUGHT:
        preferred = "Spinal Railgun";
        break;
    case HULL_TITAN:
    case HULL_FLAGSHIP:
        preferred = "Beam Lance";
        break;
    case HULL_STATION:
        preferred = "Point Defence Grid";
        break;
    default:
        break;
    }

    int index = findWeapon(preferred);

    return (index < 0) ? 0 : index;
}

int defaultLoadout(int shipClassIndex, int out[], int outMax)
{
    if (!out || outMax <= 0)
        return 0;

    const shipclassdef *hull = shipClassAt(shipClassIndex);

    if (shipClassIndex < 0 || shipClassIndex >= shipClassCount)
        return 0;

    int written = 0;

    // Rules of thumb: one reactor and engine, then proportionally split the
    // remaining slots between weapons, shields and systems.
    const char *plan[8];
    int planCount = 0;

    plan[planCount++] = "Fusion Core II";
    plan[planCount++] = "Ion Drive II";
    plan[planCount++] = "Deflector II";
    plan[planCount++] = "Ablative Plate II";

    if (hull->weaponSlots >= 4)
        plan[planCount++] = "Heavy Laser Turret";
    else
        plan[planCount++] = "Pulse Laser Turret";

    if (hull->fighterCapacity > 0)
        plan[planCount++] = "Fighter Bay I";

    if (hull->tier >= 5)
        plan[planCount++] = "Targeting Computer II";
    else
        plan[planCount++] = "Targeting Computer I";

    if (hull->defenceSlots >= 4)
        plan[planCount++] = "PD Grid II";

    for (int i = 0; i < planCount && written < outMax; ++i)
    {
        int index = findComponent(plan[i]);

        if (index >= 0)
        {
            out[written] = index;
            written++;
        }
    }

    return written;
}

/* ---------------- fleet composition ---------------- */

int fleetHullSupply(const int shipClassIndices[], int count)
{
    if (!shipClassIndices || count <= 0)
        return 0;

    int supply = 0;

    for (int i = 0; i < count; ++i)
    {
        const shipclassdef *hull = shipClassAt(shipClassIndices[i]);

        if (shipClassIndices[i] >= 0 && shipClassIndices[i] < shipClassCount)
            supply += hull->supplyUse;
    }

    return supply;
}

int recommendedEscorts(hullclass hull)
{
    switch (hull)
    {
    case HULL_DREADNOUGHT:
        return 12;
    case HULL_BATTLESHIP:
    case HULL_CARRIER:
        return 8;
    case HULL_TITAN:
    case HULL_FLAGSHIP:
        return 20;
    case HULL_STATION:
        return 6;
    default:
        return 2;
    }
}

/* ---------------- presentation ---------------- */

void showShipClassList()
{
    drawHeader("Ship Classes");

    for (int i = 0; i < shipClassCount; ++i)
    {
        const shipclassdef &s = shipClassTable[i];

        cout << "  " << i << ". " << s.name;

        int length = (int)strlen(s.name);
        for (int pad = length; pad < 24; ++pad)
            cout << ' ';

        cout << hullClassName(s.hull)
             << "  T" << s.tier
             << "  cost " << s.cost
             << "  hp " << s.hullHP << endl;
    }

    cout << endl;
}

void showShipClassDetail(int shipClassIndex)
{
    const shipclassdef *s = shipClassAt(shipClassIndex);

    if (shipClassIndex < 0 || shipClassIndex >= shipClassCount)
    {
        out("No such ship class.");
        return;
    }

    drawHeader(s->name);

    drawFieldText("Hull", hullClassName(s->hull));
    drawFieldText("Type", shipTypeName(s->type));
    drawFieldText("Role", s->role);
    drawField("Tier", s->tier);
    drawField("Cost", s->cost);

    cout << endl
         << "  Hull:" << endl;
    drawField("Hull HP", s->hullHP);
    drawField("Shield HP", s->shieldHP);
    drawField("Armor", s->armorRating);
    drawField("Evasion", s->evasion);
    drawField("Speed", s->speed);

    cout << endl
         << "  Systems:" << endl;
    drawField("Reactor output", s->reactorOutput);
    drawField("Crew required", s->crewRequired);
    drawField("Supply use", s->supplyUse);
    drawField("Maintenance", s->maintenance);

    cout << endl
         << "  Hardpoints:" << endl;
    drawField("Weapon slots", s->weaponSlots);
    drawField("Defence slots", s->defenceSlots);
    drawField("System slots", s->systemSlots);
    drawField("Fighter capacity", s->fighterCapacity);

    cout << endl;
    outWrapped(s->description);
    cout << endl;
}

void showComponentList()
{
    drawHeader("Components");

    for (int i = 0; i < componentCount; ++i)
    {
        const component &c = componentTable[i];

        cout << "  " << i << ". " << c.name;

        int length = (int)strlen(c.name);
        for (int pad = length; pad < 26; ++pad)
            cout << ' ';

        cout << componentKindName(c.kind)
             << "  T" << c.tier
             << "  cost " << c.cost
             << "  pwr " << c.powerDraw << endl;
    }

    cout << endl;
}

void showWeaponList()
{
    drawHeader("Weapons");

    for (int i = 0; i < weaponRecordCount; ++i)
    {
        const weaponprofile &w = weaponTable[i];

        cout << "  " << i << ". " << w.name;

        int length = (int)strlen(w.name);
        for (int pad = length; pad < 22; ++pad)
            cout << ' ';

        cout << weaponTypeName(w.type)
             << "  dmg " << w.damage
             << "  acc " << w.accuracy
             << "  rng " << w.range
             << "  cd " << w.cooldown << endl;
    }

    cout << endl;
}

void showShipComparison(int classA, int classB)
{
    const shipclassdef *a = shipClassAt(classA);
    const shipclassdef *b = shipClassAt(classB);

    drawHeader("Comparison");

    cout << "  " << a->name;

    int lengthA = (int)strlen(a->name);
    int lengthB = (int)strlen(b->name);
    int gap = lengthA - lengthB;

    for (int pad = 0; pad < 24 - lengthA + gap; ++pad)
        cout << ' ';

    cout << b->name << endl;

    cout << "  hull " << a->hullHP << " / " << b->hullHP << endl;
    cout << "  shield " << a->shieldHP << " / " << b->shieldHP << endl;
    cout << "  armor " << a->armorRating << " / " << b->armorRating << endl;
    cout << "  speed " << a->speed << " / " << b->speed << endl;
    cout << "  evasion " << a->evasion << " / " << b->evasion << endl;
    cout << "  slots " << a->weaponSlots << " / " << b->weaponSlots << endl;
    cout << "  cost " << a->cost << " / " << b->cost << endl;

    cout << endl;
}
