// src/core/universe.c++ -- global game data and the space map.
// This is the file that actually DEFINES the arrays declared "extern" in
// include/core/globals.h.  It must be compiled exactly once.

#include "spacebattlerpg.h"

/* ---------------- global objects ---------------- */

space spaceArray[MAX_SYSTEMS];
Technology TechnologyArray[17];
upgrads upgradeArray[17];
ship_item artifactArray[17];
ship_item glyphsArray[17];
ship_item enchantmentArray[17];
player charactership;
enemyships enemyshipsArray[17];

difficultmode difficultMode;
targetscore targetScore;

/* ---------------- space map ---------------- */

// Known star systems of the Star Trek universe, roughly ordered by the
// region of space they are associated with: the Sol sector first, then the
// core Federation worlds, the other major powers, and finally the distant
// regions reached in Voyager / Deep Space Nine / Enterprise.
//
// This table is the single source of truth for system names.  Other modules
// (src/game/galaxy.c++) read it through starSystemNames() / starSystemCount()
// so the map, the 4X layer and the menus always agree.
static const char *planetNames[MAX_SYSTEMS] = {
    /* --- Sol sector / core Federation --- */
    "Sol",              // Earth, Mars, Jupiter Station
    "Alpha Centauri",   // Zefram Cochrane's adopted home
    "Vulcan",           // 40 Eridani A; T'Khasi
    "Andor",            // Andorian homeworld
    "Tellar Prime",     // Tellarite homeworld
    "Rigel",            // Rigel system; Starbase 12
    "Betazed",          // Betazoid homeworld
    "Trill",            // Trill and the symbionts
    "Bajor",            // Bajoran homeworld, wormhole
    "Denobula",         // Denobulan homeworld
    "Coridan",          // Coridan III, dilithium-rich
    "Deneva",           // TOS "Operation -- Annihilate!"
    "Benecia",          // Benecia colony
    "Draylax",          // Draylaxian homeworld
    "Argelius",         // Argelius II
    "Cestus",           // Cestus III, Gorn outpost

    /* --- Klingon Empire --- */
    "Qo'noS",           // Kronos, Klingon homeworld
    "Khitomer",         // Khitomer Accords
    "Narendra",         // Narendra III
    "Organia",          // Organian homeworld
    "Mempa",            // Mempa sector
    "Nimbus",           // Nimbus III, "Planet of Galactic Peace"

    /* --- Romulan Star Empire --- */
    "Romulus",          // Romulan homeworld
    "Remus",            // Reman homeworld, twin of Romulus
    "Chaltok",          // Chaltok IV, Romulan colony
    "Nelvana",          // Nelvana III, contested zone
    "Galorndon Core",   // TNG "The Enemy"
    "Cheron",           // TOS "Let That Be Your Last Battlefield"

    /* --- Cardassian Union / Bajor sector --- */
    "Cardassia Prime",  // Cardassian homeworld
    "Terok Nor",        // DS9 before it became Deep Space 9
    "Chin'toka",        // Chin'toka system, Dominion War
    "Setlik",           // Setlik III massacre
    "Dorvan",           // Dorvan V, Maquis
    "Minos",            // Minos Korva
    "Celtris",          // Celtris III, TNG "Chain of Command"

    /* --- Ferengi / Orion / other Alpha Quadrant powers --- */
    "Ferenginar",       // Ferengi homeworld
    "Orion",            // Orion homeworld
    "Nausicaa",         // Nausicaan homeworld
    "Breen",            // Breen Confederacy
    "Tzenketh",         // Tzenkethi homeworld
    "Tholia",           // Tholian homeworld
    "Gorn",             // Gorn Hegemony
    "Sheliak",          // Sheliak Corporate
    "Talos",            // Talos IV
    "Mintaka",          // Mintaka III

    /* --- Delta Quadrant (Voyager) --- */
    "Ocampa",           // Ocampa homeworld, Caretaker's array
    "Talax",            // Talaxian homeworld
    "Kazon",            // Kazon space
    "Vidiian",          // Vidiian homeworld
    "Borg Prime",       // Borg homeworld region, Delta Quadrant
    "Species 8472",     // Fluidic space border
    "Hirogen",          // Hirogen hunting grounds
    "Krenim",           // Krenim Imperium, time weapons
    "Devore",           // Devore Imperium
    "Vaadwaur",         // Vaadwaur homeworld

    /* --- Gamma Quadrant / Dominion --- */
    "Idran",            // Idran system, far end of the Bajoran wormhole
    "Founders",         // Founder homeworld
    "Karemma",          // Karemma homeworld, Dominion trade
    "T-Rogoran",        // T-Rogoran, Gamma Quadrant

    /* --- Distant / historic / Enterprise era --- */
    "Wolf 359",         // Battle of Wolf 359
    "Mutara",           // Mutara Nebula, Genesis
    "Briar Patch",      // Ba'ku homeworld region
    "Epsilon Caneris III",
    "El-Adrel IV",      // TNG "Darmok"
    "Dytallix B"        // TNG "Conspiracy"
};

static const int planetNameCount =
    (int)(sizeof(planetNames) / sizeof(planetNames[0]));

// Public accessors so other modules share this exact table.
const char *starSystemNames(int index)
{
    if (index < 0 || index >= planetNameCount)
        return "";
    return planetNames[index];
}

int starSystemCount(void) { return planetNameCount; }

// The map is a simple ring: each node links to the next and the previous one.
static void fillSpace(space arr[], int count)
{
    for (int i = 0; i < count; ++i)
    {
        strncpy(arr[i].name, planetNames[i], sizeof(arr[i].name) - 1);
        arr[i].name[sizeof(arr[i].name) - 1] = '\0';

        snprintf(arr[i].description, sizeof(arr[i].description),
                 "You are in the %s system.\n", planetNames[i]);

        arr[i].forward = (i + 1) % count;
        arr[i].backward = (i - 1 + count) % count;
        arr[i].starboard = (i + 1) % count;
        arr[i].port = (i - 1 + count) % count;
    }
}

void initializespace() { fillSpace(spaceArray, planetNameCount); }
void initializemspace() { fillSpace(spaceArray, planetNameCount); }
void initializeqspace() { fillSpace(spaceArray, planetNameCount); }
void initializeuspace() { fillSpace(spaceArray, planetNameCount); }
void initializesolspace() { fillSpace(spaceArray, planetNameCount); }
