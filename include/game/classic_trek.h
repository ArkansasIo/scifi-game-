#ifndef SBW_GAME_CLASSIC_TREK_H
#define SBW_GAME_CLASSIC_TREK_H
#include <array>
#include <string>
namespace classictrek {
enum class Object : char { Empty='.', Star='*', Klingon='K', Starbase='B', Enterprise='E' };
struct Sector { Object object = Object::Empty; };
struct Quadrant { std::array<std::array<Sector,8>,8> sectors{}; bool explored=false; };
struct State {
 std::array<std::array<Quadrant,8>,8> galaxy{};
 int quadrantX=0, quadrantY=0, sectorX=0, sectorY=0;
 int energy=500, shields=0, torpedoes=3, stardate=0, deadline=15;
 int klingonsRemaining=7, basesRemaining=2;
 unsigned int rngState=1; bool gameOver=false, won=false;
};
void initialize(State&, unsigned int seed=1979u);
std::string shortRangeScan(State&);
std::string longRangeScan(State&);
std::string status(const State&);
std::string help();
std::string execute(State&, const std::string&);
void runMission();
}
#endif
