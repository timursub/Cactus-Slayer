#include "Mushroom.hpp"
#include "Cactus.hpp"
#include <cstdlib>

std::vector<Mushroom> mushrooms;

void SpawnMushroom(Position heroPos) {
    while (true) {
        int randX = rand() % 3;
        int randY = rand() % 3;
        Position candidate = {randX, randY};

        if (candidate == heroPos) continue;
        if (IsCactusAt(candidate)) continue;

        bool occupied = false;
        for (const auto& m : mushrooms) {
            if (m.pos == candidate) { occupied = true; break; }
        }
        if (occupied) continue;

        MushroomType randomType = static_cast<MushroomType>(rand() % 3);
        mushrooms.push_back({candidate, randomType});
        break;
    }
}