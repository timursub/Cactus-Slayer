#ifndef MUSHROOM_HPP
#define MUSHROOM_HPP

#include "Types.hpp"
#include <vector>

// Global vector for mushrooms on screen
extern std::vector<Mushroom> mushrooms;

void SpawnMushroom(Position heroPos);

#endif // MUSHROOM_HPP