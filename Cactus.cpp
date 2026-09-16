#include "Cactus.hpp"
#include "Mushroom.hpp" // For mushroom occupancy checks
#include <cstdlib>
#include <cmath>

// Define global vector instance
std::vector<Position> cactuses;

void SpawnCactus(Position heroPos) {
    while (true) {
        int randX = rand() % 3;
        int randY = rand() % 3; 
        Position candidatePos = {randX, randY};

        // Don't spawn near the hero
        int distToHero = std::abs(candidatePos.x - heroPos.x) + std::abs(candidatePos.y - heroPos.y);
        if (distToHero <= 1) continue;

        // Don't spawn on top of another cactus
        bool spaceOccupied = false;
        for (const auto& cactus : cactuses) {
            if (cactus == candidatePos) {
                spaceOccupied = true;
                break;
            }
        }

        // Don't spawn on top of mushroom
        for (const auto& m : mushrooms) {
            if (m.pos == candidatePos) {
                spaceOccupied = true;
                break;
            }
        }

        if (!spaceOccupied) {
            cactuses.push_back(candidatePos);
            break;
        }
    }
}

bool IsCactusAt(Position pos) {
    for (const auto& cactus : cactuses) {
        if (cactus == pos) return true;
    }
    return false;
}

bool IsNearToCactus(Position heroPos) {
    for (const auto& cactus : cactuses) {
        int dist = std::abs(heroPos.x - cactus.x) + std::abs(heroPos.y - cactus.y);
        if (dist == 1) {
            return true;
        }
    }
    return false;
}

int GetCactusAmount(int totalKills, int& outCycleCount) {
    const int baseKillRequirements[] = { 20, 25, 33, 25 };
    
    int killsLeft = totalKills;
    int cycle = 0;

    while (true) {
        float cycleMultiplier = std::pow(1.2f, cycle);

        for (int stage = 0; stage < 4; stage++) {
            int scaledThreshold = static_cast<int>(baseKillRequirements[stage] * cycleMultiplier);

            if (killsLeft < scaledThreshold) {
                outCycleCount = cycle;
                
                switch (stage) {
                    case 0: return 1;
                    case 1: return 2;
                    case 2: return 3;
                    case 3: return 2;
                }
            }
            killsLeft -= scaledThreshold;
        }

        cycle++;
    }
}