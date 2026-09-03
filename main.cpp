#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream>

// Universal fallback helper functions for saving persistent data
void SaveStorageValue(int position, int value) {
    std::ofstream outFile("save.data", std::ios::binary | std::ios::in | std::ios::out);
    if (!outFile.is_open()) {
        outFile.open("save.data", std::ios::binary | std::ios::trunc);
    }
    outFile.seekp(position * sizeof(int));
    outFile.write(reinterpret_cast<const char*>(&value), sizeof(int));
}

int LoadStorageValue(int position) {
    std::ifstream inFile("save.data", std::ios::binary);
    if (!inFile.is_open()) return 0; // Returns 0 if file doesn't exist yet
    
    inFile.seekg(position * sizeof(int));
    int value = 0;
    inFile.read(reinterpret_cast<char*>(&value), sizeof(int));
    return inFile.gcount() > 0 ? value : 0;
}

// struct to represent a position on the 3x3 grid
struct Position {
    int x;
    int y;

    // Helper operator to easily compare if two positions are equal
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};



//mushroom types
enum MushroomType {
    MSH_HP,          // +1 HP
    MSH_SHIELD,      // Shield for 15 seconds
    MSH_KILL_CACTUS  // Kills a random cactus
};

// Storage Positions for Save Data
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


//knife constructor
struct Knife {
    Position pos;
};

//mushroom constructor
struct Mushroom {
    Position pos;
    MushroomType type;
};

struct PlayerUpgrades {
    int highScore = 0;
    
    // Mushroom Levels (0 to Max)
    int shieldLvl = 0; // Level 0 = 1s, Level 6 = 7s
    int hpMushroomLvl = 0;     // Level 0 = +1 HP, Level 4 = +5 HP
    int killMushroomLvl = 0;   // Level 0 = 1 cactus, Level 1 = 2 cactuses
    int mushSpawnRateLvl = 0;      // Increases 15% spawn chance up to e.g. 35%
    int playerHpLvl = 0; // Level 0 = 5 HP, Level 5 = 10 HP
    int knifeLvl = 0; // Level 0 = 1 knife, Level 1 = 2 knives
};

struct SaveData {
    int highestScore = 0;
    PlayerUpgrades upgrades;
};

// Global vector for mushrooms on screen
std::vector<Mushroom> mushrooms;
// Global list of cactuses
std::vector<Position> cactuses;
//Global vector for Knife
std::vector<Knife> droppedKnives;


// Function to spawn a cactus on any random empty cell
void SpawnCactus(Position heroPos) {
    while (true) {
        int randX = rand() % 3; // random positions
        int randY = rand() % 3; 
        Position candidatePos = {randX, randY};

            //  Don't spawn near the hero
        int distToHero = std::abs(candidatePos.x - heroPos.x) + std::abs(candidatePos.y - heroPos.y);

        
        if (distToHero <= 1) continue;

            //  Don't spawn on top of another cactus
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

        // If space is free, add the cactus and exit loop
        if (!spaceOccupied) {
            cactuses.push_back(candidatePos);
            break;
        }
    }
}


// check if a specific position contains a cactus
bool IsCactusAt(Position pos) {
    for (const auto& cactus : cactuses) {
        if (cactus == pos) return true;
    }
    return false;
}


 //cahck if hero near cactus
bool IsNearToCactus(Position heroPos) {
    for (const auto& cactus : cactuses) {
        int dist = std::abs(heroPos.x - cactus.x) + std::abs(heroPos.y - cactus.y);
        if (dist == 1) {
            return true; // Hero landed right next to this cactus!
        }
    }
    return false;
}

// Returns true if hero currently has an active shield
bool IsShieldActive(float shieldEndTime) {
    return GetTime() < shieldEndTime;
}


// how many cactuses should be on screen based on the score
int GetCactusAmount(int score) {
    if (score >= 55000) return 3;
    if (score >= 10000)  return 2;
    return 1;
}


// game status
enum GameState {
    STATE_PLAYING,
    STATE_GAME_OVER
};

// App Scenes
enum AppScene {
    SCENE_MAIN_MENU,
    SCENE_GAME,
    SCENE_UPGRADES
};

// Movement Control Scheme
enum ControlScheme {
    CONTROL_KEYBOARD,
    CONTROL_MOUSE
};



void SpawnMushroom(Position heroPos) 
{
    while (true) 
    {
        int randX = rand() % 3;
        int randY = rand() % 3;
        Position candidate = {randX, randY};

        // Don't spawn on hero
        if (candidate == heroPos) continue;

        // Don't spawn on a cactus
        if (IsCactusAt(candidate)) continue;

        // Don't spawn on another mushroom
        bool occupied = false;
        for (const auto& m : mushrooms) {
            if (m.pos == candidate) { occupied = true; break; }
        }
        if (occupied) continue;

        // Pick a random type out of the 3
        MushroomType randomType = static_cast<MushroomType>(rand() % 3);

        mushrooms.push_back({candidate, randomType});
        break;
    }
}



// Mushroom spawn

// General cost function based on target level (Lvl 1 = 500f, Lvl 2 = 1000f, etc.)
int GetStandardUpgradeCost(int currentLvl) {
    int baseCost = 500;
    float multiplier = 1.6f;
    return static_cast<int>(baseCost * std::pow(multiplier, currentLvl));
}

// Stat Lookups based on upgrade progression levels
float GetShieldDuration(int lvl) {
    const float durations[] = { 3.0f, 5.0f, 7.0f, 9.0f, 10.0f, 13.0f, 15.0f };
    return durations[std::min(lvl, 6)];
}

int GetMaxPlayerHp(int lvl) {
    const int hpValues[] = { 3, 4, 5, 6, 7, 8, 9, 10, 15 };
    return hpValues[std::min(lvl, 8)];
}

int GetMushroomHpBonus(int lvl) {
    return std::min(lvl + 1, 10);
}

int GetMushroomCactusKills(int lvl) {
    return std::min(lvl + 1, 3);
}

int GetMushroomSpawnChance(int lvl) {
    const int chances[] = { 3, 5, 7, 10, 12, 15, 17, 20, 25 };
    return chances[std::min(lvl, 8)];
}

int GetKillMushroomCost(int lvl) {
    if (lvl == 0) return 2500;
    if (lvl == 1) return 4500;
    return 0; // Max level reached
}

void ResetGame(Position& hero, int& score, int& flowers, int& hp, int& dashCharges, 
               int& availableKnives, int& maxKnives, float& shieldEndTime, GameState& state, 
               const PlayerUpgrades& upgrades, int& streakKills, float& streakMultiplier, 
               bool& isStreakActive, int& currentGraceCharges, int maxGraceCapacity) {
    hero = {1, 1};
    score = 0;
    flowers = 0;
    hp = GetMaxPlayerHp(upgrades.playerHpLvl);
    shieldEndTime = 0.0f;
    
    cactuses.clear();
    mushrooms.clear();
    droppedKnives.clear();

    dashCharges = 3;
    maxKnives = (upgrades.knifeLvl > 0) ? 2 : 1;
    availableKnives = maxKnives;

    // RESET STREAK & GRACE STATE
    streakKills = 0;
    streakMultiplier = 1.0f;
    isStreakActive = false;
    currentGraceCharges = maxGraceCapacity;

    SpawnCactus(hero);
    state = STATE_PLAYING;
}

// Save data directly to storage file
void SaveGameData(int highestScore, const PlayerUpgrades& upgrades) {
    SaveData data;
    data.highestScore = highestScore;
    data.upgrades = upgrades;

    // SaveFileData automatically works cross-platform (and syncs on Web/Itch.io)
    SaveFileData("save.data", &data, sizeof(SaveData));
}

// Load data from storage file
void LoadGameData(int& highestScore, PlayerUpgrades& upgrades) {
    if (FileExists("save.data")) {
        int bytesRead = 0;
        unsigned char* fileData = LoadFileData("save.data", &bytesRead);

        if (fileData != NULL && bytesRead == sizeof(SaveData)) {
            SaveData* loadedData = (SaveData*)fileData;
            highestScore = loadedData->highestScore;
            upgrades = loadedData->upgrades;
        }

        // Raylib requires freeing loaded file memory after use
        UnloadFileData(fileData);
    }
}

 


int main() 
{
    
    // Seed random number generator
    srand(time(NULL));

    const int gameWidth = 360;
    const int gameHeight = 640;

    InitWindow(540, 960, "Cactus Grid - Pixel Edition");
    SetTargetFPS(60);

    //init highest score, and load save from storage
    int highestScore = 0;
    PlayerUpgrades playerUpgrades;
    LoadGameData(highestScore, playerUpgrades);

    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    
    // Hero initial position (Center)
    Position hero = {1, 1};
    int score = 0;
    int flowers = 0;
    int hp = 5;
    int dashCharges = 0;
    int movesToCactusRespawn = 0;
    int maxKnives = 1;        
    int availableKnives = 1;  
    int hitCactus = 0;
    const int MAX_DASH_CHARGES = 3; // Max dash charges
    GameState gameState = STATE_PLAYING;
    float shieldEndTime = 0; 
    //chack if we need new cactus
    bool pendingCactusRespawn = false; 
    //cahck if we need new mushroom
    int movesSinceLastCactusKill = 0;

    //Kill streak variables
    int streakKills = 0;
    float streakMultiplier = 1.0f;
    bool isStreakActive = false;

    int maxGraceCapacity = 1;     // Upgradable in shop (1, 2, 3...)
    int currentGraceCharges = 1;   // Remaining grace steps in current run

    // Persistent Saved Data
    int totalFlowers = LoadStorageValue(STORAGE_POS_FLOWERS);

    // App State Flags
    AppScene currentScene = SCENE_MAIN_MENU;
    float volume = 0.8f;
    bool isPaused = false;
    bool showSettings = false;

    // UI Buttons & Layout Rectangles
    Rectangle playBtn = { 100, 220, 160, 50 };
    Rectangle upgradesBtn = { 100, 290, 160, 50 };
    Rectangle settingsBtn = { gameWidth - 45, 10, 35, 35 };
    Rectangle backBtn = { 15, 15, 70, 30 };

    Rectangle proceedBtn = { 100, 220, 160, 45 };
    Rectangle pauseSettingsBtn = { 100, 280, 160, 45 };
    Rectangle exitBtn = { 100, 340, 160, 45 };

    Rectangle kbBtn = { 50, 200, 120, 40 };
    Rectangle msBtn = { 190, 200, 120, 40 };
    Rectangle volBar = { 50, 310, 260, 20 };          // Universal back button

    // Shop UI Button Layouts
    Rectangle btnPlayerHp  = { 20, 90,  320, 40 };
    Rectangle btnShield    = { 20, 140, 320, 40 };
    Rectangle btnMushHp    = { 20, 190, 320, 40 };
    Rectangle btnSpawnRate = { 20, 240, 320, 40 };
    Rectangle btnMushKill  = { 20, 290, 320, 40 };
    Rectangle btnKnives    = { 20, 340, 320, 40 };

    // Game Over UI Buttons
    Rectangle restartBtn = { 80, 360, 200, 50 };
    Rectangle exitGameBtn = { 80, 420, 200, 50 }; // Placed directly below restart
    
    // Spawn first Cactus
    SpawnCactus(hero);


    
        // GAME LOOP
    while (!WindowShouldClose()) 
    {
        // Calculate virtual mouse coordinates for scaled canvas UI checks
        Vector2 rawMouse = GetMousePosition();
        Vector2 mousePoint = 
        {
            rawMouse.x * ((float)gameWidth / GetScreenWidth()),
            rawMouse.y * ((float)gameHeight / GetScreenHeight())
        };

        switch (currentScene) 
        {
            case SCENE_MAIN_MENU: 
            {
                // 1. If Settings is open over Main Menu, handle Settings input ONLY
                if (showSettings) {
                    if (CheckCollisionPointRec(mousePoint, backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        showSettings = false; // Close settings overlay
                    }
                    
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePoint, volBar)) {
                        volume = (mousePoint.x - volBar.x) / volBar.width;
                        if (volume < 0.0f) volume = 0.0f;
                        if (volume > 1.0f) volume = 1.0f;
                    }
                } 
                // 2. Base Main Menu buttons (ONLY active when Settings is closed)
                else {
                    if (CheckCollisionPointRec(mousePoint, playBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        ResetGame(hero, score, flowers, hp, dashCharges, availableKnives, maxKnives, shieldEndTime, gameState, playerUpgrades, streakKills, streakMultiplier, isStreakActive, currentGraceCharges, maxGraceCapacity);
                        currentScene = SCENE_GAME;
                    }
                    if (CheckCollisionPointRec(mousePoint, upgradesBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        currentScene = SCENE_UPGRADES;
                    }
                    if (CheckCollisionPointRec(mousePoint, settingsBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        showSettings = true; // Open settings overlay over Main Menu
                    }
                }
                break;
            }

            case SCENE_UPGRADES: 
            {
                if (CheckCollisionPointRec(mousePoint, backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    currentScene = SCENE_MAIN_MENU;
                }

                // Shield Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnShield) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetStandardUpgradeCost(playerUpgrades.shieldLvl);
                    if (playerUpgrades.shieldLvl < 6 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.shieldLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // Max HP Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnPlayerHp) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetStandardUpgradeCost(playerUpgrades.playerHpLvl);
                    if (playerUpgrades.playerHpLvl < 8 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.playerHpLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // Mushroom HP Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnMushHp) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetStandardUpgradeCost(playerUpgrades.hpMushroomLvl);
                    if (playerUpgrades.hpMushroomLvl < 9 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.hpMushroomLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // Mushroom Cactus Kill Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnMushKill) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetKillMushroomCost(playerUpgrades.killMushroomLvl);
                    if (playerUpgrades.killMushroomLvl < 2 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.killMushroomLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // Spawn Rate Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnSpawnRate) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetStandardUpgradeCost(playerUpgrades.mushSpawnRateLvl);
                    if (playerUpgrades.mushSpawnRateLvl < 8 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.mushSpawnRateLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // 2 Knives Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnKnives) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = 5000;
                    if (playerUpgrades.knifeLvl < 1 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.knifeLvl = 1;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }
                break;
            }

            case SCENE_GAME: 
            {
                // 1. SETTINGS OVERLAY INPUTS (Highest Priority - opens over pause)
                if (showSettings) {
                    if (CheckCollisionPointRec(mousePoint, backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        showSettings = false; // Closes settings overlay, returning focus to Pause menu
                    }
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePoint, volBar)) {
                        volume = (mousePoint.x - volBar.x) / volBar.width;
                        if (volume < 0.0f) volume = 0.0f;
                        if (volume > 1.0f) volume = 1.0f;
                    }
                }
                // 2. PAUSE OVERLAY INPUTS
                else if (isPaused) {
                    if (CheckCollisionPointRec(mousePoint, proceedBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        isPaused = false; // Resumes gameplay
                    }
                    if (CheckCollisionPointRec(mousePoint, pauseSettingsBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        showSettings = true; // Opens settings overlay on top of pause
                    }
                    if (CheckCollisionPointRec(mousePoint, exitBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        isPaused = false;
                        currentScene = SCENE_MAIN_MENU;
                    }
                }
                // 3. GAMEPLAY INPUTS (Only evaluates when NO overlays are open)
                else {
                    // Toggle Pause Menu with Gear Button or ESC Key
                    if ((CheckCollisionPointRec(mousePoint, settingsBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
                        isPaused = true;
                    }

                    if (gameState == STATE_PLAYING) {   
                                    // INPUT AND MOVEMENT
                        Position nextHeroPos = hero;
                        bool moved = false;
                        bool isDash = false;
                        // Space key held down and shield not active
                        bool canDash = ((IsKeyDown(KEY_SPACE) || IsKeyPressed(KEY_SPACE)) && !IsShieldActive(shieldEndTime)) && (dashCharges > 0); 
                        
                        // KEYBOARD INPUTS
                        if (IsKeyPressed(KEY_D)) {
                            // Hold Space at x=0 -> Dash all the way RIGHT to x=2
                            if (canDash && hero.x == 0) { nextHeroPos.x = 2; moved = true; isDash = true; }
                            else if (hero.x < 2)          { nextHeroPos.x++; moved = true; }
                        }
                        if (IsKeyPressed(KEY_A)) {
                            // Hold Space at x=2 -> Dash all the way LEFT to x=0
                            if (canDash && hero.x == 2) { nextHeroPos.x = 0; moved = true; isDash = true; }
                            else if (hero.x > 0)          { nextHeroPos.x--; moved = true; }
                        }
                        if (IsKeyPressed(KEY_S)) {
                            // Hold Space at y=0 -> Dash all the way DOWN to y=2
                            if (canDash && hero.y == 0) { nextHeroPos.y = 2; moved = true; isDash = true; }
                            else if (hero.y < 2)          { nextHeroPos.y++; moved = true; }
                        }
                        if (IsKeyPressed(KEY_W)) {
                            // Hold Space at y=2 -> Dash all the way UP to y=0
                            if (canDash && hero.y == 2) { nextHeroPos.y = 0; moved = true; isDash = true; }
                            else if (hero.y > 0)          { nextHeroPos.y--; moved = true; }
                        }

                        // Knife Input
                        bool threwKnife = false;
                        Position knifeDir = {0, 0};

                        
                        // Only allow throwing if player has a knife and hasn't moved this frame
                        if (availableKnives > 0 && !moved) {
                            if (IsKeyPressed(KEY_Q)) { knifeDir = {-1, -1}; threwKnife = true; }
                            if (IsKeyPressed(KEY_E)) { knifeDir = { 1, -1}; threwKnife = true; }
                            if (IsKeyPressed(KEY_Z)) { knifeDir = {-1,  1}; threwKnife = true; }
                            if (IsKeyPressed(KEY_C)) { knifeDir = { 1,  1}; threwKnife = true; }
                        }

                        // MOUSE / TOUCHSCREEN INPUT
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !moved && !threwKnife) {
                            for (int r = 0; r < 3; r++) {
                                for (int c = 0; c < 3; c++) {
                                    Rectangle tileRec = { (float)(c * 100 + 30), (float)(r * 130 + 160), 100.0f, 130.0f };

                                    if (CheckCollisionPointRec(mousePoint, tileRec)) {
                                        int dx = c - hero.x;
                                        int dy = r - hero.y;
                                        int absX = std::abs(dx);
                                        int absY = std::abs(dy);

                                        // A. ADJACENT CELL CLICK (Move or Attack Cactus)
                                        if (absX + absY == 1) {
                                            nextHeroPos = { c, r };
                                            moved = true;
                                        }
                                        // B. DIAGONAL CELL CLICK (Throw Knife)
                                        else if (absX > 0 && absY > 0) {
                                            if (availableKnives > 0) {
                                                knifeDir = { (dx > 0) ? 1 : -1, (dy > 0) ? 1 : -1 };
                                                threwKnife = true;
                                            }
                                        }
                                        // C. STRAIGHT NON-ADJACENT CELL CLICK (Dash)
                                        else if ((dx == 0 || dy == 0) && (absX + absY > 1)) {
                                            if (dashCharges > 0 && !IsShieldActive(shieldEndTime)) {
                                                nextHeroPos = { c, r };
                                                moved = true;
                                                isDash = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        //Knife trajectory
                        if (threwKnife) 
                        {
                            availableKnives--;
                            Position currKnifePos = hero;
                            bool hitCactus = false; // Reset per throw

                            while (true) {
                                Position nextKnifePos = { currKnifePos.x + knifeDir.x, currKnifePos.y + knifeDir.y };

                                // Knife hits edge/wall (Miss)
                                if (nextKnifePos.x < 0 || nextKnifePos.x > 2 || nextKnifePos.y < 0 || nextKnifePos.y > 2) {
                                    droppedKnives.push_back({currKnifePos});
                                    break;
                                }

                                currKnifePos = nextKnifePos;

                                // 2. Check if knife hits a cactus
                                if (IsCactusAt(currKnifePos)) {
                                    // Destroy the cactus using C++17 remove-erase pattern
                                    cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), currKnifePos), cactuses.end());
                                    
                                    streakKills++;
                                    if (currentGraceCharges < maxGraceCapacity) {
                                        currentGraceCharges++;
                                    }

                                    // Trigger or extend streak
                                    if (streakKills >= 3) {
                                        isStreakActive = true;
                                        streakMultiplier = 1.1f + (float)(streakKills - 3) * 0.1f;
                                    }

                                    int basePoints = 500;
                                    int earnedPoints = (int)(basePoints * streakMultiplier);
                                    score += earnedPoints;
                                    flowers += 1;
                                    totalFlowers += 1; 
                                    SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                    movesSinceLastCactusKill = 0;
                                    
                                    // Knife stops and rests on the cactus's tile
                                    droppedKnives.push_back({currKnifePos});

                                    hitCactus = true;
                                    break;
                                }
                                
                            }
                            if (!hitCactus)
                                {
                                    // Non-killing move logic
                                    if (isStreakActive) {
                                        if (!IsShieldActive(shieldEndTime)) {
                                            currentGraceCharges--;
                                        }

                                        // If grace charges run out, streak ends cleanly
                                        if (currentGraceCharges < 0) {
                                            isStreakActive = false;
                                            streakKills = 0;
                                            streakMultiplier = 1.0f;
                                            currentGraceCharges = maxGraceCapacity; // Reset pool
                                        }
                                    } else {
                                        // Reset build-up counter if streak hasn't activated yet
                                        if (!IsShieldActive(shieldEndTime)) {
                                            streakKills = 0;
                                        }
                                    }
                                }
                        }


                            //Respawn cactus if destroed by mushroom
                        if (moved || isDash) 
                        {
                        
                            if (pendingCactusRespawn) 
                            {
                                movesToCactusRespawn--; // Count down 1 move
                    
                                // When 2 moves have passed, trigger the spawn and turn off flag
                                if (movesToCactusRespawn <= 0) 
                                {
                                    SpawnCactus(hero);
                                    pendingCactusRespawn = false;
                                }
                            }
                        }
 
                        if (isDash) 
                        {   
                            // 1. Consume dash charge
                            dashCharges--;

                            // 2. Calculate spatial bounds for the dash trajectory
                            int startX = std::min(hero.x, nextHeroPos.x);
                            int endX   = std::max(hero.x, nextHeroPos.x);
                            int startY = std::min(hero.y, nextHeroPos.y);
                            int endY   = std::max(hero.y, nextHeroPos.y);

                            // 3. Kill all cactuses encountered along the dash line
                            int killedCount = 0;
                            for (auto it = cactuses.begin(); it != cactuses.end(); ) {
                                if (it->x >= startX && it->x <= endX && it->y >= startY && it->y <= endY) {
                                    it = cactuses.erase(it);
                                    killedCount++;
                                } else {
                                    ++it;
                                }
                            }

                            // 4. Pre-check if destination tile contains a mushroom
                            bool landedOnMushroom = false;
                            for (const auto& m : mushrooms) {
                                if (m.pos == nextHeroPos) {
                                    landedOnMushroom = true;
                                    break;
                                }
                            }

                            // 5. Evaluate Kill rewards OR Grace penalties
                            if (killedCount > 0) {
                                streakKills += killedCount;
                                if (currentGraceCharges < maxGraceCapacity) {
                                    currentGraceCharges++;
                                }

                                if (streakKills >= 3) {
                                    isStreakActive = true;
                                    streakMultiplier = 1.1f + (float)(streakKills - 3) * 0.1f;
                                }

                                int basePoints = 500 * killedCount;
                                score += (int)(basePoints * streakMultiplier);
                                flowers += killedCount;
                                totalFlowers += killedCount;
                                SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                movesSinceLastCactusKill = 0;
                            } 
                            // ONLY deduct grace/reset streak if dash killed NOTHING AND did NOT land on a mushroom
                            else if (!landedOnMushroom) {
                                if (isStreakActive) {
                                    if (!IsShieldActive(shieldEndTime)) {
                                        currentGraceCharges--;
                                    }

                                    if (currentGraceCharges < 0) {
                                        isStreakActive = false;
                                        streakKills = 0;
                                        streakMultiplier = 1.0f;
                                        currentGraceCharges = maxGraceCapacity;
                                    }
                                } else {
                                    if (!IsShieldActive(shieldEndTime)) {
                                        streakKills = 0;
                                    }
                                }
                            }

                            // 6. Update position
                            hero = nextHeroPos;

                            // 7. Collect dropped knives if ending on knife cell
                            for (auto it = droppedKnives.begin(); it != droppedKnives.end(); ) {
                                if (it->pos == hero && !IsCactusAt(hero)) {
                                    availableKnives++;
                                    it = droppedKnives.erase(it);
                                } else {
                                    ++it;
                                }
                            }

                            // 8. Process Mushroom Pickup Effects
                            for (auto it = mushrooms.begin(); it != mushrooms.end(); ) 
                            {
                                if (it->pos == hero) 
                                {
                                    if (it->type == MSH_HP) 
                                    {
                                        hp += GetMushroomHpBonus(playerUpgrades.hpMushroomLvl);
                                    }
                                    else if (it->type == MSH_SHIELD) 
                                    {
                                        shieldEndTime = GetTime() + GetShieldDuration(playerUpgrades.shieldLvl);
                                    }
                                    else if (it->type == MSH_KILL_CACTUS) 
                                    {
                                        int killsToPerform = GetMushroomCactusKills(playerUpgrades.killMushroomLvl);
                                        int actualKills = 0;

                                        for (int k = 0; k < killsToPerform && !cactuses.empty(); k++) {
                                            int indexToKill = rand() % cactuses.size();
                                            cactuses.erase(cactuses.begin() + indexToKill);
                                            actualKills++;
                                        }

                                        if (actualKills > 0) {
                                            streakKills += actualKills;
                                            if (currentGraceCharges < maxGraceCapacity) {
                                                currentGraceCharges++;
                                            }

                                            if (streakKills >= 3) {
                                                isStreakActive = true;
                                                streakMultiplier = 1.1f + (float)(streakKills - 3) * 0.1f;
                                            }

                                            int basePoints = 500 * actualKills;
                                            score += (int)(basePoints * streakMultiplier);
                                            flowers += actualKills;
                                            totalFlowers += actualKills;
                                            SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                            movesSinceLastCactusKill = 0;
                                        }

                                        pendingCactusRespawn = true; 
                                        movesToCactusRespawn = 2;
                                    }

                                    it = mushrooms.erase(it);
                                } else {
                                    ++it;
                                }
                            }

                            // 9. Cactus proximity damage check
                            if (IsNearToCactus(hero) && !IsShieldActive(shieldEndTime)) {
                                hp--;
                            }

                            // 10. Spawn mushroom if eligible
                            if (score >= 15000 && mushrooms.empty()) {
                                int chance = GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl);
                                if ((rand() % 100) < chance) {
                                    SpawnMushroom(hero);
                                }
                            }
                        }
                        // IF MOVED
                        else if (moved)
                        {   
                            // Restore dash charge
                            if (dashCharges < MAX_DASH_CHARGES) {
                                dashCharges++;
                            }

                            // Check what is on the target tile
                            bool hitCactus = IsCactusAt(nextHeroPos);
                            
                            // Check if hero is moving onto a mushroom
                            bool hitMushroom = false;
                            for (const auto& m : mushrooms) {
                                if (m.pos == nextHeroPos) {
                                    hitMushroom = true;
                                    break;
                                }
                            }

                            if (hitCactus)
                            {
                                cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), nextHeroPos), cactuses.end()); 
                                
                                streakKills++;
                                if (currentGraceCharges < maxGraceCapacity) {
                                    currentGraceCharges++;
                                }

                                if (streakKills >= 3) {
                                    isStreakActive = true;
                                    streakMultiplier = 1.1f + (float)(streakKills - 3) * 0.1f;
                                }

                                int basePoints = 500;
                                int earnedPoints = (int)(basePoints * streakMultiplier);
                                score += earnedPoints;
                                flowers += 1;
                                totalFlowers += 1;
                                SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                movesSinceLastCactusKill = 0;
                            }
                            else 
                            {
                                hero = nextHeroPos;
                                
                                movesSinceLastCactusKill++;

                                // Apply grace penalty ONLY IF the move did NOT hit a cactus AND did NOT land on a mushroom
                                if (!hitMushroom) 
                                {
                                    if (isStreakActive) 
                                    {
                                        if (!IsShieldActive(shieldEndTime)) {
                                            currentGraceCharges--;
                                        }

                                        if (currentGraceCharges < 0) {
                                            isStreakActive = false;
                                            streakKills = 0;
                                            streakMultiplier = 1.0f;
                                            currentGraceCharges = maxGraceCapacity;
                                        }
                                    } 
                                    else 
                                    {
                                        if (!IsShieldActive(shieldEndTime)) {
                                            streakKills = 0;
                                        }
                                    }
                                }
                            }


                            // Collect dropped knives
                            for (auto it = droppedKnives.begin(); it != droppedKnives.end(); ) {
                                if (it->pos == hero && !IsCactusAt(hero)) {
                                    availableKnives++;
                                    it = droppedKnives.erase(it);
                                } else {
                                    ++it;
                                }
                            }

                            // Process Mushroom Pickup
                            for (auto it = mushrooms.begin(); it != mushrooms.end(); ) 
                            {
                                if (it->pos == hero) 
                                {
                                    if (it->type == MSH_HP) 
                                    {
                                        hp += GetMushroomHpBonus(playerUpgrades.hpMushroomLvl);
                                    }
                                    else if (it->type == MSH_SHIELD) 
                                    {
                                        shieldEndTime = GetTime() + GetShieldDuration(playerUpgrades.shieldLvl);
                                    }
                                    else if (it->type == MSH_KILL_CACTUS) 
                                    {
                                        int killsToPerform = GetMushroomCactusKills(playerUpgrades.killMushroomLvl);
                                        int actualKills = 0;

                                        for (int k = 0; k < killsToPerform && !cactuses.empty(); k++) {
                                            int indexToKill = rand() % cactuses.size();
                                            cactuses.erase(cactuses.begin() + indexToKill);
                                            actualKills++;
                                        }

                                        // Restore/extend streak upon killing cactus via mushroom
                                        if (actualKills > 0) {
                                            streakKills += actualKills;

                                            if (currentGraceCharges < maxGraceCapacity) {
                                                currentGraceCharges++;
                                            }

                                            if (streakKills >= 3) {
                                                isStreakActive = true;
                                                streakMultiplier = 1.1f + (float)(streakKills - 3) * 0.1f;
                                            }

                                            int basePoints = 500 * actualKills;
                                            score += (int)(basePoints * streakMultiplier);
                                            flowers += actualKills;
                                            totalFlowers += actualKills;
                                            SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                            movesSinceLastCactusKill = 0;
                                        }

                                        pendingCactusRespawn = true; 
                                        movesToCactusRespawn = 2;
                                    }

                                    it = mushrooms.erase(it);
                                } else {
                                    ++it;
                                }
                            }

                            // Damage check
                            if (IsNearToCactus(hero) && !IsShieldActive(shieldEndTime)) {
                                hp--;
                            }

                            // Mushroom spawn attempt
                            bool canSpawnMushroom = (movesSinceLastCactusKill < 3);
                            if (canSpawnMushroom && score >= 15000 && mushrooms.empty()) {
                                int chance = GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl);
                                if ((rand() % 100) < chance) {
                                    SpawnMushroom(hero);
                                }
                            }
                        }


                            //convert flowers to hp
                        if(flowers >= 3)
                        {
                            hp++;
                            score = score + 250;
                            flowers = flowers - 3;
                        }

                            //updates highest score
                        if (score > highestScore) {
                            highestScore = score;
                            SaveGameData(highestScore, playerUpgrades); 
                        }
                    
                            //Calculates how many cactuses should be on screen
                        int targetCactuses = GetCactusAmount(score);


                        // Hold off spawning extra cactuses while waiting for the timer
                        if (pendingCactusRespawn) {
                            targetCactuses--;
                        }

                        

                        // Fill up to the target amount
                        while ((int)cactuses.size() < targetCactuses) 
                        {
                            SpawnCactus(hero);
                        }

                            // end the game
                        if (hp <= 0) 
                        {
                            gameState = STATE_GAME_OVER;
                        }
                    

                    }
                    else if (gameState == STATE_GAME_OVER) {
                        bool mouseRestartClicked = CheckCollisionPointRec(mousePoint, restartBtn) && 
                                                (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
                        bool keyRestartPressed = IsKeyPressed(KEY_ENTER);

                        bool mouseExitClicked = CheckCollisionPointRec(mousePoint, exitGameBtn) && 
                                                (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT));

                        if (mouseRestartClicked || keyRestartPressed) {
                            ResetGame(hero, score, flowers, hp, dashCharges, availableKnives, maxKnives, shieldEndTime, gameState, playerUpgrades, streakKills, streakMultiplier, isStreakActive, currentGraceCharges, maxGraceCapacity);
                        }
                        else if (mouseExitClicked) {
                            currentScene = SCENE_MAIN_MENU;
                        }
                    }
                    
                }
                break;
            }
        } // End of switch(currentScene)


        // LAMBDA FOR RENDER
        auto DrawGameScene = [&]() {
            DrawText(TextFormat("HI-SCORE: %d", highestScore), 15, 15, 18, GOLD);
            DrawText(TextFormat("SCORE: %d", score), 15, 40, 18, BLACK);
            DrawText(TextFormat("HP: %d", hp), 15, 65, 18, RED);

            DrawText("DASH:", 15, 90, 14, DARKBLUE);
            for (int i = 0; i < MAX_DASH_CHARGES; i++) {
                Color dashColor = (i < dashCharges) ? BLUE : DARKGRAY;
                DrawRectangle(65 + (i * 18), 92, 14, 10, dashColor);
                DrawRectangleLines(65 + (i * 18), 92, 14, 10, WHITE);
            }
            
            DrawText(TextFormat("KNIVES: %d/%d", availableKnives, maxKnives), 220, 90, 14, BROWN);

            DrawText("FLOWERS", 250, 18, 14, DARKGREEN);
            for (int i = 0; i < 3; i++) {
                int slotX = 250 + (i * 30);
                int slotY = 50;
                Color flowerColor = (i < flowers) ? PINK : DARKGRAY;
                DrawCircle(slotX, slotY, 9, flowerColor);
                DrawCircleLines(slotX, slotY, 10, WHITE);
            }

            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    DrawRectangleLines((c * 100)+30, (r * 130) + 160, 100, 130, DARKGRAY);
                }
            }

            for (const auto& cactus : cactuses) {
                int cactusPixelX = cactus.x * 100 + 45;
                int cactusPixelY = cactus.y * 130 + 185;
                DrawRectangle(cactusPixelX, cactusPixelY, 70, 80, DARKGREEN);
            }

            for (const auto& k : droppedKnives) {
                int kX = k.pos.x * 100 + 72;
                int kY = k.pos.y * 130 + 215;
                DrawRectangle(kX, kY, 16, 20, BROWN);
                DrawTriangle({ (float)kX, (float)kY }, { (float)kX + 16, (float)kY }, { (float)kX + 8, (float)kY - 12 }, LIGHTGRAY);
            }

            int heroPixelX = hero.x * 100 + 80;
            int heroPixelY = hero.y * 130 + 225;
            DrawCircle(heroPixelX, heroPixelY, 22, BLUE);

            for (const auto& m : mushrooms) {
                int mPixelX = m.pos.x * 100 + 80;
                int mPixelY = m.pos.y * 130 + 225;
                Color mColor = PURPLE;
                if (m.type == MSH_HP) mColor = RED;
                else if (m.type == MSH_SHIELD) mColor = SKYBLUE;
                else if (m.type == MSH_KILL_CACTUS) mColor = ORANGE;

                DrawCircle(mPixelX, mPixelY, 14, mColor);
            }

            if (isStreakActive) {
                int centerX = gameWidth / 2;

                // Multiplier Text
                const char* streakText = TextFormat("STREAK x%.1f!", streakMultiplier);
                int streakTextWidth = MeasureText(streakText, 20);
                DrawText(streakText, centerX - (streakTextWidth / 2), 118, 20, GOLD);

                // Grace Text (Shows "INF" when shield is active)
                const char* graceText = IsShieldActive(shieldEndTime) 
                    ? "GRACE: INF" 
                    : TextFormat("GRACE: %d/%d", currentGraceCharges, maxGraceCapacity);
                    
                int graceTextWidth = MeasureText(graceText, 14);
                Color graceColor = IsShieldActive(shieldEndTime) ? SKYBLUE : ORANGE;
                DrawText(graceText, centerX - (graceTextWidth / 2), 138, 14, graceColor);
            }

            if (IsShieldActive(shieldEndTime)) {
                DrawCircleLines(heroPixelX, heroPixelY, 28, SKYBLUE);
            }

            if (gameState == STATE_GAME_OVER) {
            DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.7f));
            DrawText("GAME OVER", 60, 260, 36, RED);

            // 1. Restart Button
            Color restartBtnColor = CheckCollisionPointRec(mousePoint, restartBtn) ? LIGHTGRAY : DARKGRAY;
            DrawRectangleRec(restartBtn, restartBtnColor);
            DrawRectangleLinesEx(restartBtn, 3, WHITE);
            DrawText("RESTART", restartBtn.x + 40, restartBtn.y + 14, 24, WHITE);

            // 2. Red Exit to Menu Button (matching Pause Menu styling)
            Color exitBtnColor = CheckCollisionPointRec(mousePoint, exitGameBtn) ? MAROON : RED;
            DrawRectangleRec(exitGameBtn, exitBtnColor);
            DrawRectangleLinesEx(exitGameBtn, 3, WHITE);
            DrawText("EXIT TO MENU", exitGameBtn.x + 18, exitGameBtn.y + 16, 18, WHITE);
        }
        };

        
        // RENDER
        BeginTextureMode(target);
            ClearBackground(GRAY);

            switch (currentScene) 
            {
                case SCENE_MAIN_MENU: 
                {
                    // 1. Draw Base Main Menu UI
                    DrawText("CACTUS GRID", 50, 100, 32, DARKGREEN);
                    
                    DrawRectangleRec(playBtn, DARKBLUE);
                    DrawText("PLAY", playBtn.x + 50, playBtn.y + 12, 24, WHITE);

                    DrawRectangleRec(upgradesBtn, DARKGREEN);
                    DrawText("SHOP", upgradesBtn.x + 48, upgradesBtn.y + 12, 24, WHITE);

                    DrawRectangleRec(settingsBtn, DARKGRAY);
                    DrawText("*", settingsBtn.x + 10, settingsBtn.y + 5, 30, WHITE);

                    //Draw Flowers count
                    DrawText("CACTUS GRID", 50, 100, 32, DARKGREEN);
                    // Render lifetime bank balance at the top left of the menu
                    DrawText(TextFormat("FLOWERS: %d", totalFlowers), 15, 15, 18, PINK);

                    // 2. Draw Settings Overlay on top of Main Menu if open
                    if (showSettings) {
                        // Darken the background menu
                        DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.85f));

                        // Back button & Title
                        DrawRectangleRec(backBtn, DARKGRAY);
                        DrawText("< BACK", backBtn.x + 8, backBtn.y + 6, 16, WHITE);
                        DrawText("SETTINGS", 120, 20, 24, WHITE);
                        
                        
                        // Volume bar
                        DrawText(TextFormat("Volume: %d%%", (int)(volume * 100)), 50, 280, 18, WHITE);
                        DrawRectangleRec(volBar, LIGHTGRAY);
                        DrawRectangle(volBar.x, volBar.y, volBar.width * volume, volBar.height, BLUE);
                        DrawRectangleLinesEx(volBar, 2, WHITE);
                    }
                    break;
                }

                case SCENE_UPGRADES: 
                {
                    DrawRectangleRec(backBtn, DARKGRAY);
                    DrawText("< BACK", backBtn.x + 8, backBtn.y + 6, 16, WHITE);
                    DrawText("UPGRADES SHOP", 95, 20, 22, GOLD);
                    DrawText(TextFormat("FLOWERS: %d", totalFlowers), 120, 55, 18, PINK);

                    // Lambda helper for rendering uniform shop rows with transition format
                    auto DrawUpgradeRow = [&](Rectangle rec, const char* label, std::string val, int cost, bool maxed) {
                        DrawRectangleRec(rec, maxed ? DARKGRAY : (totalFlowers >= cost ? DARKGREEN : GRAY));
                        DrawRectangleLinesEx(rec, 2, WHITE);
                        DrawText(label, rec.x + 8, rec.y + 12, 13, WHITE);
                        
                        // Displays current -> next when upgrading, or just current when maxed
                        DrawText(val.c_str(), rec.x + 115, rec.y + 12, 13, YELLOW);

                        if (maxed) {
                            DrawText("MAX", rec.x + 265, rec.y + 12, 14, RED);
                        } else {
                            DrawText(TextFormat("%df", cost), rec.x + 250, rec.y + 12, 13, PINK);
                        }
                    };

                    
                    // 1. Max HP
                    bool hpMax = playerUpgrades.playerHpLvl >= 8;
                    std::string hpTxt = hpMax 
                        ? TextFormat("%d HP", GetMaxPlayerHp(playerUpgrades.playerHpLvl))
                        : TextFormat("%d -> %d HP", GetMaxPlayerHp(playerUpgrades.playerHpLvl), GetMaxPlayerHp(playerUpgrades.playerHpLvl + 1));
                    DrawUpgradeRow(btnPlayerHp, "Max HP", hpTxt, GetStandardUpgradeCost(playerUpgrades.playerHpLvl), hpMax);

                    // 2. Shield Duration
                    bool shieldMax = playerUpgrades.shieldLvl >= 6;
                    std::string shieldTxt = shieldMax 
                        ? TextFormat("%.0fs", GetShieldDuration(playerUpgrades.shieldLvl))
                        : TextFormat("%.0fs -> %.0fs", GetShieldDuration(playerUpgrades.shieldLvl), GetShieldDuration(playerUpgrades.shieldLvl + 1));
                    DrawUpgradeRow(btnShield, "Shield Dur.", shieldTxt, GetStandardUpgradeCost(playerUpgrades.shieldLvl), shieldMax);


                    // 3. Mushroom HP
                    bool mushHpMax = playerUpgrades.hpMushroomLvl >= 9;
                    std::string mushHpTxt = mushHpMax 
                        ? TextFormat("+%d HP", GetMushroomHpBonus(playerUpgrades.hpMushroomLvl))
                        : TextFormat("+%d -> +%d", GetMushroomHpBonus(playerUpgrades.hpMushroomLvl), GetMushroomHpBonus(playerUpgrades.hpMushroomLvl + 1));
                    DrawUpgradeRow(btnMushHp, "Mush HP", mushHpTxt, GetStandardUpgradeCost(playerUpgrades.hpMushroomLvl), mushHpMax);

                    // 4. Mushroom Kills
                    bool mushKillMax = playerUpgrades.killMushroomLvl >= 2;
                    std::string mushKillTxt = mushKillMax 
                        ? TextFormat("%d Cacti", GetMushroomCactusKills(playerUpgrades.killMushroomLvl))
                        : TextFormat("%d -> %d", GetMushroomCactusKills(playerUpgrades.killMushroomLvl), GetMushroomCactusKills(playerUpgrades.killMushroomLvl + 1));
                    DrawUpgradeRow(btnMushKill, "Mush Kills", mushKillTxt, GetKillMushroomCost(playerUpgrades.killMushroomLvl), mushKillMax);

                    // 5. Mushroom Spawn Rate
                    bool spawnMax = playerUpgrades.mushSpawnRateLvl >= 8;
                    std::string spawnTxt = spawnMax 
                        ? TextFormat("%d%%", GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl))
                        : TextFormat("%d%% -> %d%%", GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl), GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl + 1));
                    DrawUpgradeRow(btnSpawnRate, "Mush Spawn", spawnTxt, GetStandardUpgradeCost(playerUpgrades.mushSpawnRateLvl), spawnMax);

                    // 6. 2 Knives
                    bool knifeMax = playerUpgrades.knifeLvl >= 1;
                    std::string knifeTxt = knifeMax ? "Knives" : "1 -> 2";
                    DrawUpgradeRow(btnKnives, "Knives", knifeTxt, 5000, knifeMax);

                    break;
                }

                case SCENE_GAME: 
                {
                    // 1. ALWAYS execute the drawing lambda first to render game world
                    DrawGameScene();

                    if (gameState != STATE_GAME_OVER) {
                        DrawRectangleRec(settingsBtn, DARKGRAY);
                        DrawText("*", settingsBtn.x + 10, settingsBtn.y + 5, 30, WHITE);
                    }

                    // 2. Render Settings Overlay ON TOP of everything if open
                    if (showSettings) {
                        DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.85f));
                        DrawRectangleRec(backBtn, DARKGRAY);
                        DrawText("< BACK", backBtn.x + 8, backBtn.y + 6, 16, WHITE);
                        DrawText("SETTINGS", 120, 20, 24, WHITE);

                       
                        DrawText(TextFormat("Volume: %d%%", (int)(volume * 100)), 50, 280, 18, WHITE);
                        DrawRectangleRec(volBar, LIGHTGRAY);
                        DrawRectangle(volBar.x, volBar.y, volBar.width * volume, volBar.height, BLUE);
                        DrawRectangleLinesEx(volBar, 2, WHITE);
                    }
                    // 3. Render Pause Overlay ON TOP of game if paused
                    else if (isPaused) {
                        DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.6f));
                        DrawText("PAUSED", 115, 140, 32, WHITE);

                        DrawRectangleRec(proceedBtn, DARKBLUE);
                        DrawText("PROCEED", proceedBtn.x + 30, proceedBtn.y + 12, 20, WHITE);

                        DrawRectangleRec(pauseSettingsBtn, DARKGRAY);
                        DrawText("SETTINGS", pauseSettingsBtn.x + 25, pauseSettingsBtn.y + 12, 20, WHITE);

                        DrawRectangleRec(exitBtn, RED);
                        DrawText("EXIT TO MENU", exitBtn.x + 10, exitBtn.y + 12, 18, WHITE);
                    }
                    break;
                }
            
                

                
            }
            
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);

            Rectangle srcRec = { 0.0f, 0.0f, (float)gameWidth, (float)-gameHeight };
            Rectangle destRec = { 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() };

            DrawTexturePro(target.texture, srcRec, destRec, { 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
