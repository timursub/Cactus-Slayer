#ifndef CACTUS_HPP
#define CACTUS_HPP

#include "Types.hpp"
#include <vector>

// Global list of cactuses on screen
extern std::vector<Position> cactuses;

// Cactus management & spatial checks
void SpawnCactus(Position heroPos);
bool IsCactusAt(Position pos);
bool IsNearToCactus(Position heroPos);
int GetCactusAmount(int totalKills, int& outCycleCount);

#endif // CACTUS_HPP