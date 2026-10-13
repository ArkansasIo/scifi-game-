# Classic Star Trek Tactical Mission

This is a clean-room C++17 tactical mission inspired by the 1979 Star Trek BASIC
game family. It does not copy a historical source listing; period implementations
differed in exact counts and rules.

## Mission rules
- Galaxy: 8 × 8 quadrants; each quadrant has an 8 × 8 sector grid.
- Starting state: 7 Klingons, 75 stars, 2 starbases, 500 energy and 3 torpedoes.
- Deadline: 15 stardates. Destroy all Klingons before energy or time runs out.
- Starbases replenish energy and torpedoes when the Enterprise is adjacent.

## Commands
- `NAV dx dy`: move by sector offsets; quadrant crossing costs extra energy and one stardate.
- `SRS`: current quadrant sector scan.
- `LRS`: local 3 × 3 quadrant KBS scan.\n- `DAM`: damage-control report; combat can damage ship systems.\n- `COM option`: library computer reports (0–5: galaxy, status, torpedo help, starbase navigation, direction/distance, quadrant map).
- `PHA energy`: spend energy on phasers against local Klingons.
- `TOR row col`: fire at a sector in the current quadrant (coordinates 1–8).
- `SHE energy`: transfer energy into shields.
- `DOCK`: resupply at a nearby starbase.
- `STATUS`, `HELP`, `QUIT`: status, help, abandon mission.

## Build and test
The module is part of the standard PowerShell build list and CMake target.
Run `cmake -S . -B build`, `cmake --build build`, then
`ctest --test-dir build --output-on-failure`.

## Historical reference
The April 1979 *Practical Computing* version describes 64 quadrants, 64 sectors
per quadrant, 75 stars, seven Klingons, two starbases, 500 energy, three photon
torpedoes and a 15-stardate mission. Other versions differ:
https://worldradiohistory.com/UK/Practical-Computing/70s/Practical-Computing-1979-04-S-OCR.pdf
