#ifndef TYPES_HPP
#define TYPES_HPP

#include "raylib.h"
#include <cmath>
#include <cstdlib>

// Grid Position
struct Position {
    int x;
    int y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

// Mushroom Types
enum MushroomType {
    MSH_HP,          // +1 HP
    MSH_SHIELD,      // Shield duration
    MSH_KILL_CACTUS  // Kills a random cactus
};

// Storage Keys & Positions
typedef enum {
    STORAGE_POS_HISCORE = 0,
    STORAGE_POS_FLOWERS = 1
} StoragePosition;

enum StorageKey {
    STORAGE_HIGHEST_SCORE = 0,
    STORAGE_SHIELD_LVL    = 1,
    STORAGE_HP_MUSH_LVL   = 2,
    STORAGE_KILL_MUSH_LVL = 3,
    STORAGE_SPAWN_RATE_LVL= 4,
    STORAGE_PLAYER_HP_LVL = 5,
    STORAGE_KNIFE_LVL     = 6
};

// Game Status Enums
enum GameState {
    STATE_PLAYING,
    STATE_GAME_OVER
};

enum AppScene {
    SCENE_MAIN_MENU,
    SCENE_GAME,
    SCENE_UPGRADES
};

enum ControlScheme {
    CONTROL_KEYBOARD,
    CONTROL_MOUSE
};

// Entity Data Structures
struct Knife {
    Position pos;
};

struct Mushroom {
    Position pos;
    MushroomType type;
};

struct PlayerUpgrades {
    int highScore = 0;
    int shieldLvl = 0;
    int hpMushroomLvl = 0;
    int killMushroomLvl = 0;
    int mushSpawnRateLvl = 0;
    int playerHpLvl = 0;
    int knifeLvl = 0;
};

struct SaveData {
    int highestScore = 0;
    PlayerUpgrades upgrades;
};

struct GameContext {
    Position hero = {1, 1};
    int score = 0;
    int flowers = 0;
    int hp = 5;
    int dashCharges = 3;
    int availableKnives = 1;
    int maxKnives = 1;
    float shieldEndTime = 0.0f;
    GameState state = STATE_PLAYING;
    int consecutiveEmptyMoves = 0;
    
    // Streak & Grace
    int streakKills = 0;
    float streakMultiplier = 1.0f;
    bool isStreakActive = false;
    float streakTimeRemaining = 5.0f; 
    int currentGraceCharges = 1;
    int maxGraceCapacity = 1;

    // Wave Progression & Kills Tracking
    int totalCactiKilled = 0;
    int cycleCount = 0;

    // Respawn & Mushroom Trackers
    int movesToCactusRespawn = 0;
    bool pendingCactusRespawn = false;
    int movesSinceLastCactusKill = 0;
};

#endif // TYPES_HPP