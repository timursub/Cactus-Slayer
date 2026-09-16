#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>
#include "Types.hpp"
#include "SaveSystem.hpp"
#include "Cactus.hpp"
#include "Mushroom.hpp"
#include "Knife.hpp"

// Full Combat AI Controller
void ExecuteBotAction(GameContext& ctx, bool isAggressive) {
    // -------------------------------------------------------------
    // PRIORITY 1: THROW KNIFE (Diagonal Cacti)
    // -------------------------------------------------------------
    if (ctx.availableKnives > 0) {
        Position diagonals[4] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
        for (const auto& dir : diagonals) {
            Position target = { ctx.hero.x + dir.x, ctx.hero.y + dir.y };
            if (target.x >= 0 && target.x < 3 && target.y >= 0 && target.y < 3) {
                if (IsCactusAt(target)) {
                    // Throw Knife
                    ctx.availableKnives--;
                    cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), target), cactuses.end());
                    
                    ctx.streakKills++;
                    ctx.totalCactiKilled++;
                    if (ctx.streakKills >= 3) {
                        if (!ctx.isStreakActive) {
                            ctx.isStreakActive = true;
                            ctx.streakTimeRemaining = 4.0f;
                        }
                        ctx.streakMultiplier = 2.1f + (float)(ctx.streakKills - 3) * 0.1f;
                    }

                    int basePoints = 250;
                    ctx.score += (int)(basePoints * ctx.streakMultiplier);
                    ctx.flowers++;
                    
                    // Knife rests at target location
                    droppedKnives.push_back({target});
                    return;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PRIORITY 2: RETRIEVE DROPPED KNIVES (Adjacent Knife Cells)
    // -------------------------------------------------------------
    Position adjDirs[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
    for (const auto& dir : adjDirs) {
        Position target = { ctx.hero.x + dir.x, ctx.hero.y + dir.y };
        if (target.x >= 0 && target.x < 3 && target.y >= 0 && target.y < 3) {
            for (auto it = droppedKnives.begin(); it != droppedKnives.end(); ++it) {
                if (it->pos == target && !IsCactusAt(target)) {
                    ctx.hero = target;
                    ctx.availableKnives++;
                    droppedKnives.erase(it);
                    return;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PRIORITY 3: DASHING (Straight Line Attacks)
    // -------------------------------------------------------------
    if (ctx.dashCharges > 0 && (isAggressive || IsShieldActive(ctx.shieldEndTime))) {
        // Evaluate straight dash lines (Distance of 2)
        Position dashTargets[4] = {
            { (ctx.hero.x == 0) ? 2 : -1, ctx.hero.y }, // Horizontal Right
            { (ctx.hero.x == 2) ? 0 : -1, ctx.hero.y }, // Horizontal Left
            { ctx.hero.x, (ctx.hero.y == 0) ? 2 : -1 }, // Vertical Down
            { ctx.hero.x, (ctx.hero.y == 2) ? 0 : -1 }  // Vertical Up
        };

        for (const auto& dashPos : dashTargets) {
            if (dashPos.x != -1 && dashPos.y != -1) {
                // Check if any cactus exists along the line
                int startX = std::min(ctx.hero.x, dashPos.x);
                int endX   = std::max(ctx.hero.x, dashPos.x);
                int startY = std::min(ctx.hero.y, dashPos.y);
                int endY   = std::max(ctx.hero.y, dashPos.y);

                int cactiInPath = 0;
                for (const auto& cactus : cactuses) {
                    if (cactus.x >= startX && cactus.x <= endX && cactus.y >= startY && cactus.y <= endY) {
                        cactiInPath++;
                    }
                }

                if (cactiInPath > 0) {
                    // Execute Dash
                    ctx.dashCharges--;
                    for (auto it = cactuses.begin(); it != cactuses.end(); ) {
                        if (it->x >= startX && it->x <= endX && it->y >= startY && it->y <= endY) {
                            it = cactuses.erase(it);
                            ctx.totalCactiKilled++;
                        } else {
                            ++it;
                        }
                    }

                    ctx.streakKills += cactiInPath;
                    if (ctx.streakKills >= 3) {
                        if (!ctx.isStreakActive) {
                            ctx.isStreakActive = true;
                            ctx.streakTimeRemaining = 4.0f;
                        }
                        ctx.streakMultiplier = 2.1f + (float)(ctx.streakKills - 3) * 0.1f;
                    }

                    int basePoints = 250 * cactiInPath;
                    ctx.score += (int)(basePoints * ctx.streakMultiplier);
                    ctx.flowers += cactiInPath;
                    ctx.hero = dashPos;
                    return;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // PRIORITY 4: STANDARD MOVEMENT / ATTACK
    // -------------------------------------------------------------
    std::vector<Position> validMoves;
    for (const auto& dir : adjDirs) {
        Position nx = { ctx.hero.x + dir.x, ctx.hero.y + dir.y };
        if (nx.x >= 0 && nx.x < 3 && nx.y >= 0 && nx.y < 3) {
            validMoves.push_back(nx);
        }
    }

    // Recharge Dash Charge on normal moves
    if (ctx.dashCharges < 3) ctx.dashCharges++;

    // A. Attack Cactus if Aggressive or Shielded
    if (isAggressive || IsShieldActive(ctx.shieldEndTime)) {
        for (const auto& move : validMoves) {
            if (IsCactusAt(move)) {
                cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), move), cactuses.end());
                ctx.streakKills++;
                ctx.totalCactiKilled++;
                ctx.score += (int)(250 * ctx.streakMultiplier);
                ctx.flowers++;
                return;
            }
        }
    }

    // B. Safe Moves (Avoid Cacti)
    std::vector<Position> safeMoves;
    for (const auto& move : validMoves) {
        if (!IsCactusAt(move) && !IsNearToCactus(move)) safeMoves.push_back(move);
    }
    if (!safeMoves.empty()) {
        ctx.hero = safeMoves[rand() % safeMoves.size()];
        return;
    }

    // C. Non-Cactus Empty Tiles
    std::vector<Position> emptyMoves;
    for (const auto& move : validMoves) {
        if (!IsCactusAt(move)) emptyMoves.push_back(move);
    }
    if (!emptyMoves.empty()) {
        ctx.hero = emptyMoves[rand() % emptyMoves.size()];
        return;
    }

    // Fallback: Attack Adjacent Cactus
    Position finalMove = validMoves[rand() % validMoves.size()];
    if (IsCactusAt(finalMove)) {
        cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), finalMove), cactuses.end());
        ctx.score += 250;
        ctx.flowers++;
    } else {
        ctx.hero = finalMove;
    }
}

void RunSimulation(int totalRuns, bool aggressiveStrategy) {
    long long totalScore = 0;
    long long totalMoves = 0;
    long long highestRunScore = 0;

    PlayerUpgrades maxUpgrades = {5, 5, 2, 2, 5, 1};

    for (int i = 0; i < totalRuns; i++) {
        GameContext ctx;
        ctx.hero = {1, 1};
        ctx.hp = GetMaxPlayerHp(maxUpgrades.playerHpLvl);
        ctx.dashCharges = 3;
        ctx.availableKnives = 1;
        ctx.maxKnives = 1;
        ctx.maxGraceCapacity = 1;
        ctx.currentGraceCharges = 1;
        ctx.state = STATE_PLAYING;
        
        cactuses.clear();
        mushrooms.clear();
        droppedKnives.clear();
        SpawnCactus(ctx.hero);

        int moves = 0;
        while (ctx.state == STATE_PLAYING && moves < 2000) {
            ExecuteBotAction(ctx, aggressiveStrategy);

            // Proximity damage
            if (IsNearToCactus(ctx.hero) && !IsShieldActive(ctx.shieldEndTime)) {
                ctx.hp--;
            }

            // Flower healing conversion
            if (ctx.flowers >= 3) {
                ctx.hp++;
                ctx.score += 200;
                ctx.flowers -= 3;
            }

            // Spawn dynamic cacti
            int dummyCycle = 0;
            int targetCacti = GetCactusAmount(ctx.totalCactiKilled, dummyCycle);
            while ((int)cactuses.size() < targetCacti) {
                SpawnCactus(ctx.hero);
            }

            if (ctx.hp <= 0) ctx.state = STATE_GAME_OVER;
            moves++;
        }

        totalScore += ctx.score;
        totalMoves += moves;
        if (ctx.score > highestRunScore) highestRunScore = ctx.score;
    }

    std::cout << (aggressiveStrategy ? "=== ADVANCED AGGRESSIVE BOT ===" : "=== ADVANCED PASSIVE BOT ===") << std::endl;
    std::cout << "Avg Score: " << (totalScore / totalRuns) << std::endl;
    std::cout << "High Score: " << highestRunScore << std::endl;
    std::cout << "Avg Survival Moves: " << (totalMoves / totalRuns) << std::endl << std::endl;
}

int main() {
    srand(time(NULL));
    std::cout << "Running 10,000 Full Combat Simulations...\n\n";
    RunSimulation(10000, false); // Advanced Passive
    RunSimulation(10000, true);  // Advanced Aggressive
    return 0;
}