// include/world/universe.h -- the space map.
#ifndef SBW_WORLD_UNIVERSE_H
#define SBW_WORLD_UNIVERSE_H

void initializespace();
void initializemspace();
void initializeqspace();
void initializeuspace();
void initializesolspace();

/* Known Star Trek star-system name table (defined in src/core/universe.c++). */
const char *starSystemNames(int index);
int starSystemCount(void);

#endif
