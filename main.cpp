#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream>

#include "Animation.hpp"
#include "AssetManager.hpp"
#include "Types.hpp"
#include "SaveSystem.hpp"
#include "Cactus.hpp"
#include "Mushroom.hpp"
#include "Knife.hpp"



float GetNextPositionStep(float current, float target, float maxDelta) {
    if (current < target) return std::min(current + maxDelta, target);
    if (current > target) return std::max(current - maxDelta, target);
    return target;
}

void ResetGame(GameContext& ctx, const PlayerUpgrades& upgrades) {
    ctx.hero = {1, 1};
    ctx.score = 0;
    ctx.flowers = 0;
    ctx.hp = GetMaxPlayerHp(upgrades.playerHpLvl);
    ctx.shieldEndTime = 0.0f;
    ctx.state = STATE_PLAYING;
    ctx.consecutiveEmptyMoves = 0;
    
    cactuses.clear();
    mushrooms.clear();
    droppedKnives.clear();

    ctx.dashCharges = 3;
    ctx.maxKnives = (upgrades.knifeLvl > 0) ? 2 : 1;
    ctx.availableKnives = ctx.maxKnives;

    // Reset Streak & Grace State
    ctx.streakKills = 0;
    ctx.streakMultiplier = 1.0f;
    ctx.isStreakActive = false;
    ctx.streakTimeRemaining = 0.0f;
    ctx.maxGraceCapacity = 1;
    ctx.currentGraceCharges = ctx.maxGraceCapacity;

    // Reset Wave Trackers
    ctx.totalCactiKilled = 0;
    ctx.cycleCount = 0;
    ctx.movesToCactusRespawn = 0;
    ctx.pendingCactusRespawn = false;
    ctx.movesSinceLastCactusKill = 0;

    SpawnCactus(ctx.hero);
}



 


int main() 
{
    
    // Seed random number generator
    srand(time(NULL));

    const int gameWidth = 360;
    const int gameHeight = 640;

    InitWindow(540, 960, "Cactus Grid - Pixel Edition");
    SetTargetFPS(60);

    LoadAllGameAssets(); // Replaces all individual LoadTexture calls
        
    // Load saved game data
    int highestScore = 0;
    PlayerUpgrades playerUpgrades;
    LoadGameData(highestScore, playerUpgrades);

    // Draw background
    DrawTexturePro(
        g_Assets.bgMain,
        Rectangle{ 0.0f, 0.0f, (float)g_Assets.bgMain.width, (float)g_Assets.bgMain.height },
        Rectangle{ 0.0f, 0.0f, (float)gameWidth, (float)gameHeight },
        Vector2{ 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    // Hero initial position (Center)
    Position hero = {1, 1};
    int score = 0;
    int flowers = 0;
    int hp = 5;
    int dashCharges = 0;
    int movesToCactusRespawn = 0;
    int maxKnives = 1;        
    int availableKnives = 1;  
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
    float streakTimeRemaining = 5.0f;

    int maxGraceCapacity = 1;     // Upgradable in shop (1, 2, 3...)
    int currentGraceCharges = 1;   // Remaining grace steps in current run

    // Persistent Saved Data
    int totalFlowers = LoadStorageValue(STORAGE_POS_FLOWERS);

    // Load saved game data
    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);


    Vector2 currentVisualPos = { 
    hero.x * 100.0f + 30.0f + 50.0f, 
    hero.y * 130.0f + 160.0f + 65.0f 
    };

    // Movement speed in pixels per second (we can tweak this later!)
    float moveSpeed = 1000.0f;


    // App State Flags
    AppScene currentScene = SCENE_MAIN_MENU;
    float volume = 0.8f;
    bool isPaused = false;
    bool showSettings = false;

    GameContext gameCtx;
/*
    playerUpgrades.playerHpLvl     = 5; // 8 Max HP
    playerUpgrades.shieldLvl       = 5; // 7s Shield
    playerUpgrades.hpMushroomLvl   = 2; // +3 HP Mushroom
    playerUpgrades.killMushroomLvl = 2; // 3 Cacti Mushroom
    playerUpgrades.mushSpawnRateLvl= 5; // 7% Spawn Rate
    playerUpgrades.knifeLvl        = 1; // 2 Knives Enabled
    totalFlowers                   = 99999; // Infinite Flowers
*/
    // Add this right after initializing your local variables in main()
    auto SyncVarsFromContext = [&]() {
        hero = gameCtx.hero;
        score = gameCtx.score;
        flowers = gameCtx.flowers;
        hp = gameCtx.hp;
        dashCharges = gameCtx.dashCharges;
        availableKnives = gameCtx.availableKnives;
        maxKnives = gameCtx.maxKnives;
        shieldEndTime = gameCtx.shieldEndTime;
        gameState = gameCtx.state;
        movesSinceLastCactusKill = gameCtx.movesSinceLastCactusKill;

        streakKills = gameCtx.streakKills;
        streakMultiplier = gameCtx.streakMultiplier;
        isStreakActive = gameCtx.isStreakActive;
        streakTimeRemaining = gameCtx.streakTimeRemaining;
        currentGraceCharges = gameCtx.currentGraceCharges;
        maxGraceCapacity = gameCtx.maxGraceCapacity;
        

        movesToCactusRespawn = gameCtx.movesToCactusRespawn;
        pendingCactusRespawn = gameCtx.pendingCactusRespawn;
        movesSinceLastCactusKill = gameCtx.movesSinceLastCactusKill;
    };

    auto SyncContextFromVars = [&]() {
        gameCtx.hero = hero;
        gameCtx.score = score;
        gameCtx.flowers = flowers;
        gameCtx.hp = hp;
        gameCtx.dashCharges = dashCharges;
        gameCtx.availableKnives = availableKnives;
        gameCtx.maxKnives = maxKnives;
        gameCtx.shieldEndTime = shieldEndTime;
        gameCtx.state = gameState;
        gameCtx.movesSinceLastCactusKill = movesSinceLastCactusKill;

        gameCtx.streakKills = streakKills;
        gameCtx.streakMultiplier = streakMultiplier;
        gameCtx.isStreakActive = isStreakActive;
        gameCtx.streakTimeRemaining = streakTimeRemaining;
        gameCtx.currentGraceCharges = currentGraceCharges;
        gameCtx.maxGraceCapacity = maxGraceCapacity;

        gameCtx.movesToCactusRespawn = movesToCactusRespawn;
        gameCtx.pendingCactusRespawn = pendingCactusRespawn;
        gameCtx.movesSinceLastCactusKill = movesSinceLastCactusKill;
    };

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
    ResetGame(gameCtx, playerUpgrades);
    SyncVarsFromContext();
    currentVisualPos = Vector2{ hero.x * 100.0f + 80.0f, hero.y * 130.0f + 225.0f };

    
    Vector2 rawMouse = { 0 };
    Vector2 mousePoint = { 0 };

    // Lambda to draw base background + active scene overlay layer
    auto DrawBackgroundWithOverlay = [&](Texture2D overlayTex) {
        // 1. Base background
        DrawTexturePro(
            g_Assets.bgMain,
            Rectangle{ 0.0f, 0.0f, (float)g_Assets.bgMain.width, (float)g_Assets.bgMain.height },
            Rectangle{ 0.0f, 0.0f, (float)gameWidth, (float)gameHeight },
            Vector2{ 0.0f, 0.0f }, 0.0f, WHITE
        );

        // 2. Scene overlay layer
        if (overlayTex.id > 0) {
            DrawTexturePro(
                overlayTex,
                Rectangle{ 0.0f, 0.0f, (float)overlayTex.width, (float)overlayTex.height },
                Rectangle{ 0.0f, 0.0f, (float)gameWidth, (float)gameHeight },
                Vector2{ 0.0f, 0.0f }, 0.0f, WHITE
            );
        }
    };

    
        // GAME LOOP
    while (!WindowShouldClose()) 
    {
        float dt = GetFrameTime();

        // Update animation frame timers every frame
        UpdateAssetAnimations(dt);

        // Target pixel coordinates of hero's current logical grid tile
        // 1. Target pixel coordinates of hero's current logical grid tile
        Vector2 targetVisualPos = {
            hero.x * 100.0f + 80.0f,
            hero.y * 130.0f + 225.0f
        };

        // 2. Compute delta and remaining distance
        float slideDx = targetVisualPos.x - currentVisualPos.x;
        float slideDy = targetVisualPos.y - currentVisualPos.y;
        float slideDist = std::abs(slideDx) + std::abs(slideDy);

        // 3. Keep movement state active during slide without resetting animStateTimer every frame
        if (slideDist > 2.0f) {
            bool isMovingState = (g_Assets.currentHeroState == PLAYER_ANIM_MOVE_RIGHT ||
                                  g_Assets.currentHeroState == PLAYER_ANIM_MOVE_LEFT  ||
                                  g_Assets.currentHeroState == PLAYER_ANIM_MOVE_UP    ||
                                  g_Assets.currentHeroState == PLAYER_ANIM_MOVE_DOWN);

            if (!isMovingState) {
                if (std::abs(slideDx) > std::abs(slideDy)) {
                    if (slideDx > 0) SetPlayerAnimationState(PLAYER_ANIM_MOVE_RIGHT, 0.25f);
                    else             SetPlayerAnimationState(PLAYER_ANIM_MOVE_LEFT, 0.25f);
                } else {
                    if (slideDy > 0) SetPlayerAnimationState(PLAYER_ANIM_MOVE_DOWN, 0.25f);
                    else             SetPlayerAnimationState(PLAYER_ANIM_MOVE_UP, 0.25f);
                }
            }
        } else {
            // Once destination tile is reached, return state to idle if still in movement state
            if (g_Assets.currentHeroState == PLAYER_ANIM_MOVE_RIGHT ||
                g_Assets.currentHeroState == PLAYER_ANIM_MOVE_LEFT  ||
                g_Assets.currentHeroState == PLAYER_ANIM_MOVE_UP    ||
                g_Assets.currentHeroState == PLAYER_ANIM_MOVE_DOWN) {
                g_Assets.currentHeroState = PLAYER_ANIM_IDLE;
            }
        }

        // 4. Smoothly step visual position toward target grid tile
        currentVisualPos.x = GetNextPositionStep(currentVisualPos.x, targetVisualPos.x, moveSpeed * dt);
        currentVisualPos.y = GetNextPositionStep(currentVisualPos.y, targetVisualPos.y, moveSpeed * dt);

        
        // Re-assign existing variables without re-declaring Vector2
        rawMouse = GetMousePosition();
        mousePoint = 
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
                        // Resetting on menu play or game restart
                        ResetGame(gameCtx, playerUpgrades);
                        SyncVarsFromContext();
                        currentVisualPos = Vector2{ hero.x * 100.0f + 80.0f, hero.y * 130.0f + 225.0f };
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
                    if (playerUpgrades.shieldLvl < 5 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.shieldLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // Max HP Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnPlayerHp) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetStandardUpgradeCost(playerUpgrades.playerHpLvl);
                    if (playerUpgrades.playerHpLvl < 5 && totalFlowers >= cost) {
                        totalFlowers -= cost;
                        playerUpgrades.playerHpLvl++;
                        SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                        SaveGameData(highestScore, playerUpgrades);
                    }
                }

                // Mushroom HP Upgrade Click
                if (CheckCollisionPointRec(mousePoint, btnMushHp) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int cost = GetStandardUpgradeCost(playerUpgrades.hpMushroomLvl);
                    if (playerUpgrades.hpMushroomLvl < 2 && totalFlowers >= cost) {
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
                    if (playerUpgrades.mushSpawnRateLvl < 5 && totalFlowers >= cost) {
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
                            if (canDash && hero.x == 0) {
                                nextHeroPos.x = 2; moved = true; isDash = true;
                                SetPlayerAnimationState(PLAYER_ANIM_DASH, 0.35f);
                            } else if (hero.x < 2) {
                                nextHeroPos.x++; moved = true;
                                SetPlayerAnimationState(PLAYER_ANIM_MOVE_RIGHT, 0.25f);
                            }
                        }
                        if (IsKeyPressed(KEY_A)) {
                            if (canDash && hero.x == 2) {
                                nextHeroPos.x = 0; moved = true; isDash = true;
                                SetPlayerAnimationState(PLAYER_ANIM_DASH, 0.35f);
                            } else if (hero.x > 0) {
                                nextHeroPos.x--; moved = true;
                                SetPlayerAnimationState(PLAYER_ANIM_MOVE_LEFT, 0.25f);
                            }
                        }
                        if (IsKeyPressed(KEY_S)) {
                            if (canDash && hero.y == 0) {
                                nextHeroPos.y = 2; moved = true; isDash = true;
                                SetPlayerAnimationState(PLAYER_ANIM_DASH, 0.35f);
                            } else if (hero.y < 2) {
                                nextHeroPos.y++; moved = true;
                                SetPlayerAnimationState(PLAYER_ANIM_MOVE_DOWN, 0.25f);
                            }
                        }
                        if (IsKeyPressed(KEY_W)) {
                            if (canDash && hero.y == 2) {
                                nextHeroPos.y = 0; moved = true; isDash = true;
                                SetPlayerAnimationState(PLAYER_ANIM_DASH, 0.35f);
                            } else if (hero.y > 0) {
                                nextHeroPos.y--; moved = true;
                                SetPlayerAnimationState(PLAYER_ANIM_MOVE_UP, 0.25f);
                            }
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

                                        // A. ADJACENT CELL CLICK (Move)
                                        if (absX + absY == 1) {
                                            nextHeroPos = { c, r };
                                            moved = true;

                                            if (dx > 0)      SetPlayerAnimationState(PLAYER_ANIM_MOVE_RIGHT, 0.25f);
                                            else if (dx < 0) SetPlayerAnimationState(PLAYER_ANIM_MOVE_LEFT, 0.25f);
                                            else if (dy > 0) SetPlayerAnimationState(PLAYER_ANIM_MOVE_DOWN, 0.25f);
                                            else if (dy < 0) SetPlayerAnimationState(PLAYER_ANIM_MOVE_UP, 0.25f);
                                        }
                                        // B. DIAGONAL CELL CLICK (Throw Knife)
                                        else if (absX > 0 && absY > 0) {
                                            if (availableKnives > 0) {
                                                knifeDir = { (dx > 0) ? 1 : -1, (dy > 0) ? 1 : -1 };
                                                threwKnife = true;
                                                SetPlayerAnimationState(PLAYER_ANIM_THROW, 0.25f);
                                            }
                                        }
                                        // C. STRAIGHT NON-ADJACENT CLICK (Dash)
                                        else if ((dx == 0 || dy == 0) && (absX + absY > 1)) {
                                            if (dashCharges > 0 && !IsShieldActive(shieldEndTime)) {
                                                nextHeroPos = { c, r };
                                                moved = true;
                                                isDash = true;
                                                SetPlayerAnimationState(PLAYER_ANIM_DASH, 0.35f);
                                            }
                                        }
                                    }
                                }
                            }
                        } 

                        if (!showSettings && !isPaused && gameState == STATE_PLAYING) {
                            // Run timer countdown every frame
                            if (isStreakActive) {
                                streakTimeRemaining -= GetFrameTime();
                                if (streakTimeRemaining <= 0.0f) {
                                    isStreakActive = false;
                                    streakKills = 0;
                                    streakMultiplier = 1.0f;
                                    streakTimeRemaining = 0.0f;
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
                                    gameCtx.consecutiveEmptyMoves = 0;
                                    // Destroy the cactus using C++17 remove-erase pattern
                                    cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), currKnifePos), cactuses.end());
                                    
                                    streakKills++;
                                    gameCtx.totalCactiKilled++;
                                    if (currentGraceCharges < maxGraceCapacity) {
                                        currentGraceCharges++;
                                    }

                                    // Trigger or extend streak
                                    if (streakKills >= 3) {
                                        if (!isStreakActive) {
                                            isStreakActive = true;
                                            streakTimeRemaining = 4.0f;
                                        } else {
                                            streakTimeRemaining = std::min(streakTimeRemaining + 1.0f, 4.0f); // Capped at 4s max
                                        }
                                        streakMultiplier = 2.1f + (float)(streakKills - 3) * 0.1f;
                                    }

                                    int basePoints = 250;
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
                                    gameCtx.consecutiveEmptyMoves++;
                                    if (gameCtx.consecutiveEmptyMoves >= 3) {
                                        if (!IsShieldActive(shieldEndTime)) {
                                            hp--;
                                        }
                                        gameCtx.consecutiveEmptyMoves = 0;
                                    }
                                    
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
                                    gameCtx.totalCactiKilled++;
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

                            // Right above: else if (!landedOnMushroom)
                            if (landedOnMushroom) {
                                gameCtx.consecutiveEmptyMoves = 0; // <--- ADD THIS LINE
                            }

                            // 5. Evaluate Kill rewards OR Grace penalties
                            if (killedCount > 0) {
                                gameCtx.consecutiveEmptyMoves = 0;
                                streakKills += killedCount;
                                if (currentGraceCharges < maxGraceCapacity) {
                                    currentGraceCharges++;
                                }

                                if (streakKills >= 3) {
                                    if (!isStreakActive) {
                                        isStreakActive = true;
                                        streakTimeRemaining = 4.0f;
                                    } else {
                                        streakTimeRemaining = std::min(streakTimeRemaining + 1.0f, 4.0f); // Capped at 4s max
                                    }
                                    streakMultiplier = 2.1f + (float)(streakKills - 3) * 0.1f;
                                }

                                int basePoints = 250 * killedCount;
                                score += (int)(basePoints * streakMultiplier);
                                flowers += killedCount;
                                totalFlowers += killedCount;
                                SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                movesSinceLastCactusKill = 0;
                            } 
                            // ONLY deduct grace/reset streak if dash killed NOTHING AND did NOT land on a mushroom
                            else if (!landedOnMushroom) {

                                gameCtx.consecutiveEmptyMoves++;
                            if (gameCtx.consecutiveEmptyMoves >= 3) {
                                    if (!IsShieldActive(shieldEndTime)) {
                                        hp--; // Inflict 1 damage on 2nd empty move
                                    }
                                    gameCtx.consecutiveEmptyMoves = 0; // Reset counter after penalty tick
                                }
                                
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
                                        shieldEndTime = (float)GetTime() + GetShieldDuration(playerUpgrades.shieldLvl);
                                    }
                                    else if (it->type == MSH_KILL_CACTUS) 
                                    {
                                        int killsToPerform = GetMushroomCactusKills(playerUpgrades.killMushroomLvl);
                                        int actualKills = 0;
                                        gameCtx.totalCactiKilled++;

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
                                                if (!isStreakActive) {
                                                    isStreakActive = true;
                                                    streakTimeRemaining = 4.0f;
                                                } else {
                                                    streakTimeRemaining = std::min(streakTimeRemaining + 1.0f, 4.0f); // Capped at 4s max
                                                }
                                                streakMultiplier = 2.1f + (float)(streakKills - 3) * 0.1f;
                                            }

                                            int basePoints = 250 * actualKills;
                                            score += (int)(basePoints * streakMultiplier);
                                            flowers += actualKills;
                                            totalFlowers += actualKills;
                                            SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                            movesSinceLastCactusKill = 0;
                                        }

                                        // --- DYNAMIC STAGE-AWARE INSTANT RESPAWN FIX ---
                                        int dummyCycle = 0;
                                        int targetCactuses = GetCactusAmount(gameCtx.totalCactiKilled, dummyCycle);

                                        // If board is empty AND target count for current stage is 1, spawn immediately regardless of cycle count
                                        if (cactuses.empty() && targetCactuses == 1) {
                                            SpawnCactus(hero);
                                            pendingCactusRespawn = false;
                                            movesToCactusRespawn = 0;
                                        } else {
                                            // Multi-cactus stages keep the delayed timer
                                            pendingCactusRespawn = true; 
                                            movesToCactusRespawn = 2;
                                        }
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

                            // Right above: else if (!landedOnMushroom)
                            if (hitMushroom) {
                                gameCtx.consecutiveEmptyMoves = 0; // <--- ADD THIS LINE
                            }

                            if (hitCactus)
                            {
                                gameCtx.consecutiveEmptyMoves = 0;
                                cactuses.erase(std::remove(cactuses.begin(), cactuses.end(), nextHeroPos), cactuses.end()); 
                                
                                streakKills++;
                                gameCtx.totalCactiKilled++;
                                if (currentGraceCharges < maxGraceCapacity) {
                                    currentGraceCharges++;
                                }

                                if (streakKills >= 3) {
                                    if (!isStreakActive) {
                                        isStreakActive = true;
                                        streakTimeRemaining = 4.0f;
                                    } else {
                                        streakTimeRemaining = std::min(streakTimeRemaining + 1.0f, 4.0f); // Capped at 4s max
                                    }
                                    streakMultiplier = 2.1f + (float)(streakKills - 3) * 0.1f;
                                }

                                int basePoints = 250;
                                int earnedPoints = (int)(basePoints * streakMultiplier);
                                score += earnedPoints;
                                flowers += 1;
                                totalFlowers += 1;
                                SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                movesSinceLastCactusKill = 0;
                            }
                            else 
                            {
                                
                                movesSinceLastCactusKill++;

                                // EMPTY STEP (NO KILL): Increment stall counter and inflict damage on Step 2
                                gameCtx.consecutiveEmptyMoves++;
                                if (gameCtx.consecutiveEmptyMoves >= 3) {
                                    if (!IsShieldActive(shieldEndTime)) {
                                        hp--; // Inflict 1 damage on 2nd empty move
                                    }
                                    gameCtx.consecutiveEmptyMoves = 0; // Reset counter after penalty tick
                                }

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
                                else 
                                {
                                    //empty step logic
                                    movesSinceLastCactusKill++;
                                    
                                }
                            }


                                hero = nextHeroPos;


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
                                        shieldEndTime = (float)GetTime() + GetShieldDuration(playerUpgrades.shieldLvl);
                                    }
                                    else if (it->type == MSH_KILL_CACTUS) 
                                    {
                                        int killsToPerform = GetMushroomCactusKills(playerUpgrades.killMushroomLvl);
                                        int actualKills = 0;
                                        gameCtx.totalCactiKilled++;

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
                                                if (!isStreakActive) {
                                                    isStreakActive = true;
                                                    streakTimeRemaining = 4.0f;
                                                } else {
                                                    streakTimeRemaining = std::min(streakTimeRemaining + 1.0f, 4.0f); // Capped at 4s max
                                                }
                                                streakMultiplier = 3.1f + (float)(streakKills - 3) * 0.1f;
                                            }

                                            int basePoints = 250 * actualKills;
                                            score += (int)(basePoints * streakMultiplier);
                                            flowers += actualKills;
                                            totalFlowers += actualKills;
                                            SaveStorageValue(STORAGE_POS_FLOWERS, totalFlowers);
                                            movesSinceLastCactusKill = 0;
                                        }

                                        // --- DYNAMIC STAGE-AWARE INSTANT RESPAWN FIX ---
                                        int dummyCycle = 0;
                                        int targetCactuses = GetCactusAmount(gameCtx.totalCactiKilled, dummyCycle);

                                        // If board is empty AND target count for current stage is 1, spawn immediately regardless of cycle count
                                        if (cactuses.empty() && targetCactuses == 1) {
                                            SpawnCactus(hero);
                                            pendingCactusRespawn = false;
                                            movesToCactusRespawn = 0;
                                        } else {
                                            // Multi-cactus stages keep the delayed timer
                                            pendingCactusRespawn = true; 
                                            movesToCactusRespawn = 2;
                                        }
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

                        


                            //convert flowers to hp
                        if(flowers >= 3)
                        {
                            hp++;
                            score = score + 200;
                            flowers = flowers - 3;
                        }

                            //updates highest score
                        if (score > highestScore) {
                            highestScore = score;
                            SaveGameData(highestScore, playerUpgrades); 
                        }
                    
                            //Calculates how many cactuses should be on screen
                        // Replaces previous GetCactusAmount call in SCENE_GAME:
                        int targetCactuses = GetCactusAmount(gameCtx.totalCactiKilled, gameCtx.cycleCount);


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
                            // Resetting on menu play or game restart
                            ResetGame(gameCtx, playerUpgrades);
                            SyncVarsFromContext();
                            currentVisualPos = Vector2{ hero.x * 100.0f + 80.0f, hero.y * 130.0f + 225.0f };
                        }
                        else if (mouseExitClicked) {
                            currentScene = SCENE_MAIN_MENU;
                        }
                    }

                    
                }
                SyncContextFromVars();
                break;
            }
        } // End of switch(currentScene)

// LAMBDA FOR RENDER
        auto DrawGameScene = [&]() {
            // HUD Top Bar
            DrawTextEx(g_Assets.mainFont, TextFormat("HI-SCORE: %d", highestScore), Vector2{ 15.0f, 15.0f }, 15.0f, 1.0f, GOLD);
            DrawTextEx(g_Assets.mainFont, TextFormat("SCORE: %d", score), Vector2{ 15.0f, 38.0f }, 15.0f, 1.0f, BLACK);
            DrawTextEx(g_Assets.mainFont, TextFormat("HP: %d", hp), Vector2{ 15.0f, 61.0f }, 15.0f, 1.0f, RED);

            // Dash Charges
            DrawTextEx(g_Assets.mainFont, "DASH:", Vector2{ 15.0f, 85.0f }, 12.0f, 1.0f, DARKBLUE);
            for (int i = 0; i < MAX_DASH_CHARGES; i++) {
                Color dashColor = (i < dashCharges) ? BLUE : DARKGRAY;
                DrawRectangle(60 + (i * 16), 87, 12, 9, dashColor);
                DrawRectangleLines(60 + (i * 16), 87, 12, 9, WHITE);
            }
            
            // Knives Counter
            DrawTextEx(g_Assets.mainFont, TextFormat("KNIVES: %d/%d", availableKnives, maxKnives), Vector2{ 220.0f, 85.0f }, 12.0f, 1.0f, BROWN);

            // Flowers Indicators
            DrawTextEx(g_Assets.mainFont, "FLOWERS", Vector2{ 250.0f, 15.0f }, 12.0f, 1.0f, DARKGREEN);
            for (int i = 0; i < 3; i++) {
                int slotX = 250 + (i * 28);
                int slotY = 45;
                Color flowerColor = (i < flowers) ? PINK : DARKGRAY;
                DrawCircle(slotX, slotY, 8, flowerColor);
                DrawCircleLines(slotX, slotY, 9, WHITE);
            }

            // Grid Lines
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    DrawRectangleLines((c * 100) + 30, (r * 130) + 160, 100, 130, DARKGRAY);
                }
            }

            // Cacti Entities
            for (const auto& cactus : cactuses) {
                int cactusPixelX = cactus.x * 100 + 45;
                int cactusPixelY = cactus.y * 130 + 185;
                DrawRectangle(cactusPixelX, cactusPixelY, 70, 80, DARKGREEN);
            }

            // Dropped Knives Entities
            for (const auto& k : droppedKnives) {
                int kX = k.pos.x * 100 + 72;
                int kY = k.pos.y * 130 + 215;
                DrawRectangle(kX, kY, 16, 20, BROWN);
                DrawTriangle({ (float)kX, (float)kY }, { (float)kX + 16, (float)kY }, { (float)kX + 8, (float)kY - 12 }, LIGHTGRAY);
            }

            // HERO
            Vector2 visualHeroPos = currentVisualPos;

            // Draw Animated Hero
            switch (g_Assets.currentHeroState) {
                case PLAYER_ANIM_MOVE_RIGHT:
                    g_Assets.playerMoveRight.Draw(visualHeroPos, 72.0f);
                    break;
                case PLAYER_ANIM_MOVE_LEFT:
                    g_Assets.playerMoveRight.Draw(visualHeroPos, 72.0f, WHITE, true);
                    break;
                case PLAYER_ANIM_MOVE_UP:
                    g_Assets.playerMoveUp.Draw(visualHeroPos, 72.0f);
                    break;
                case PLAYER_ANIM_MOVE_DOWN:
                    g_Assets.playerMoveDown.Draw(visualHeroPos, 72.0f);
                    break;
                default:
                    g_Assets.playerIdle.Draw(visualHeroPos, 72.0f);
                    break;
            }

            // Draw Shield attached to smooth visual position
            if (IsShieldActive(shieldEndTime)) {
                DrawCircleLines((int)visualHeroPos.x, (int)visualHeroPos.y, 28, SKYBLUE);
            }

            // Mushroom Entities
            for (const auto& m : mushrooms) {
                int mPixelX = m.pos.x * 100 + 80;
                int mPixelY = m.pos.y * 130 + 225;
                Color mColor = PURPLE;
                if (m.type == MSH_HP) mColor = RED;
                else if (m.type == MSH_SHIELD) mColor = SKYBLUE;
                else if (m.type == MSH_KILL_CACTUS) mColor = ORANGE;

                DrawCircle(mPixelX, mPixelY, 14, mColor);
            }

            // Cycle Tracker
            DrawTextEx(g_Assets.mainFont, TextFormat("CYCLE: %d", gameCtx.cycleCount + 1), Vector2{ 15.0f, 108.0f }, 12.0f, 1.0f, DARKPURPLE);

            // Streak & Grace Charges HUD
            if (gameCtx.isStreakActive) {
                float centerX = (float)gameWidth / 2.0f;
                float fontSize = 16.0f;

                Color streakColor = (gameCtx.streakTimeRemaining <= 2.0f) ? ORANGE : GOLD;

                // Centered streak text using MeasureTextEx
                const char* streakText = TextFormat("STREAK x%.1f!", gameCtx.streakMultiplier);
                Vector2 streakTextSize = MeasureTextEx(g_Assets.mainFont, streakText, fontSize, 1.0f);
                float textX = centerX - (streakTextSize.x / 2.0f);
                float textY = 108.0f;

                DrawTextEx(g_Assets.mainFont, streakText, Vector2{ textX, textY }, fontSize, 1.0f, streakColor);

                // Centered shrinking timer bar
                const float maxStreakDuration = 4.0f;
                float fillRatio = gameCtx.streakTimeRemaining / maxStreakDuration;
                if (fillRatio > 1.0f) fillRatio = 1.0f;
                if (fillRatio < 0.0f) fillRatio = 0.0f;

                float fullLineWidth = streakTextSize.x;
                float currentLineWidth = fullLineWidth * fillRatio;

                float lineY = textY + fontSize + 2.0f;
                float lineHeight = 2.0f;
                float lineX = centerX - (currentLineWidth / 2.0f);

                DrawRectangle((int)lineX, (int)lineY, (int)currentLineWidth, (int)lineHeight, streakColor);

                // Grace Charges Indicator
                const char* graceText = IsShieldActive(shieldEndTime) 
                    ? "GRACE: INF" 
                    : TextFormat("GRACE: %d/%d", gameCtx.currentGraceCharges, gameCtx.maxGraceCapacity);
                    
                Vector2 graceTextSize = MeasureTextEx(g_Assets.mainFont, graceText, 12.0f, 1.0f);
                Color graceColor = IsShieldActive(shieldEndTime) ? SKYBLUE : ORANGE;
                
                DrawTextEx(g_Assets.mainFont, graceText, Vector2{ centerX - (graceTextSize.x / 2.0f), lineY + 6.0f }, 12.0f, 1.0f, graceColor);
            }

            // Game Over Overlay
            if (gameState == STATE_GAME_OVER) {
                DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.7f));
                
                // Centered Game Over Title
                Vector2 gameOverSize = MeasureTextEx(g_Assets.mainFont, "GAME OVER", 30.0f, 1.0f);
                DrawTextEx(g_Assets.mainFont, "GAME OVER", Vector2{ ((float)gameWidth - gameOverSize.x) / 2.0f, 260.0f }, 30.0f, 1.0f, RED);

                // 1. Restart Button
                Color restartBtnColor = CheckCollisionPointRec(mousePoint, restartBtn) ? LIGHTGRAY : DARKGRAY;
                DrawRectangleRec(restartBtn, restartBtnColor);
                DrawRectangleLinesEx(restartBtn, 3, WHITE);
                
                Vector2 restartTextSize = MeasureTextEx(g_Assets.mainFont, "RESTART", 20.0f, 1.0f);
                DrawTextEx(g_Assets.mainFont, "RESTART", Vector2{ restartBtn.x + (restartBtn.width - restartTextSize.x) / 2.0f, restartBtn.y + 15.0f }, 20.0f, 1.0f, WHITE);

                // 2. Exit to Menu Button
                Color exitBtnColor = CheckCollisionPointRec(mousePoint, exitGameBtn) ? MAROON : RED;
                DrawRectangleRec(exitGameBtn, exitBtnColor);
                DrawRectangleLinesEx(exitGameBtn, 3, WHITE);

                Vector2 exitTextSize = MeasureTextEx(g_Assets.mainFont, "EXIT TO MENU", 15.0f, 1.0f);
                DrawTextEx(g_Assets.mainFont, "EXIT TO MENU", Vector2{ exitGameBtn.x + (exitGameBtn.width - exitTextSize.x) / 2.0f, exitGameBtn.y + 17.0f }, 15.0f, 1.0f, WHITE);
            }
        };
        
        // RENDER
        BeginTextureMode(target);

        switch (currentScene) 
            {
                case SCENE_MAIN_MENU: 
                {
                    DrawBackgroundWithOverlay(g_Assets.bgOverlayMenu);
                    
                    // Buttons
                    DrawRectangleRec(playBtn, DARKBLUE);
                    DrawTextEx(g_Assets.mainFont, "PLAY", Vector2{ playBtn.x + 50.0f, playBtn.y + 14.0f }, 20.0f, 1.0f, WHITE);

                    DrawRectangleRec(upgradesBtn, DARKGREEN);
                    DrawTextEx(g_Assets.mainFont, "SHOP", Vector2{ upgradesBtn.x + 48.0f, upgradesBtn.y + 14.0f }, 20.0f, 1.0f, WHITE);

                    DrawRectangleRec(settingsBtn, DARKGRAY);
                    DrawTextEx(g_Assets.mainFont, "*", Vector2{ settingsBtn.x + 10.0f, settingsBtn.y + 5.0f }, 26.0f, 1.0f, WHITE);

                    // Title Header
                    DrawTextEx(g_Assets.mainFont, "CACTUS GRID", Vector2{ 50.0f, 100.0f }, 26.0f, 1.0f, DARKGREEN);
                    
                    // Lifetime Flowers Balance
                    DrawTextEx(g_Assets.mainFont, TextFormat("FLOWERS: %d", totalFlowers), Vector2{ 15.0f, 15.0f }, 15.0f, 1.0f, PINK);

                    // 2. Settings Overlay (Main Menu)
                    if (showSettings) {
                        DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.85f));

                        DrawRectangleRec(backBtn, DARKGRAY);
                        DrawTextEx(g_Assets.mainFont, "< BACK", Vector2{ backBtn.x + 8.0f, backBtn.y + 7.0f }, 14.0f, 1.0f, WHITE);
                        DrawTextEx(g_Assets.mainFont, "SETTINGS", Vector2{ 120.0f, 20.0f }, 20.0f, 1.0f, WHITE);
                        
                        DrawTextEx(g_Assets.mainFont, TextFormat("Volume: %d%%", (int)(volume * 100)), Vector2{ 50.0f, 280.0f }, 15.0f, 1.0f, WHITE);
                        DrawRectangleRec(volBar, LIGHTGRAY);
                        DrawRectangle(volBar.x, volBar.y, volBar.width * volume, volBar.height, BLUE);
                        DrawRectangleLinesEx(volBar, 2, WHITE);
                    }
                    break;
                }

                case SCENE_UPGRADES: 
                {
                    DrawBackgroundWithOverlay(g_Assets.bgOverlayShop);

                    DrawRectangleRec(backBtn, DARKGRAY);
                    DrawTextEx(g_Assets.mainFont, "< BACK", Vector2{ backBtn.x + 8.0f, backBtn.y + 7.0f }, 14.0f, 1.0f, WHITE);
                    DrawTextEx(g_Assets.mainFont, "UPGRADES SHOP", Vector2{ 100.0f, 20.0f }, 18.0f, 1.0f, GOLD);
                    DrawTextEx(g_Assets.mainFont, TextFormat("FLOWERS: %d", totalFlowers), Vector2{ 120.0f, 55.0f }, 15.0f, 1.0f, PINK);

                    auto DrawUpgradeRow = [&](Rectangle rec, const char* label, std::string val, int cost, bool maxed) {
                        DrawRectangleRec(rec, maxed ? DARKGRAY : (totalFlowers >= cost ? DARKGREEN : GRAY));
                        DrawRectangleLinesEx(rec, 2, WHITE);
                        DrawTextEx(g_Assets.mainFont, label, Vector2{ rec.x + 8.0f, rec.y + 13.0f }, 12.0f, 1.0f, WHITE);
                        
                        DrawTextEx(g_Assets.mainFont, val.c_str(), Vector2{ rec.x + 115.0f, rec.y + 13.0f }, 12.0f, 1.0f, YELLOW);

                        if (maxed) {
                            DrawTextEx(g_Assets.mainFont, "MAX", Vector2{ rec.x + 265.0f, rec.y + 13.0f }, 12.0f, 1.0f, RED);
                        } else {
                            DrawTextEx(g_Assets.mainFont, TextFormat("%df", cost), Vector2{ rec.x + 250.0f, rec.y + 13.0f }, 12.0f, 1.0f, PINK);
                        }
                    };

                    bool hpMax = playerUpgrades.playerHpLvl >= 8;
                    std::string hpTxt = hpMax 
                        ? TextFormat("%d HP", GetMaxPlayerHp(playerUpgrades.playerHpLvl))
                        : TextFormat("%d -> %d HP", GetMaxPlayerHp(playerUpgrades.playerHpLvl), GetMaxPlayerHp(playerUpgrades.playerHpLvl + 1));
                    DrawUpgradeRow(btnPlayerHp, "Max HP", hpTxt, GetStandardUpgradeCost(playerUpgrades.playerHpLvl), hpMax);

                    bool shieldMax = playerUpgrades.shieldLvl >= 6;
                    std::string shieldTxt = shieldMax 
                        ? TextFormat("%.0fs", GetShieldDuration(playerUpgrades.shieldLvl))
                        : TextFormat("%.0fs -> %.0fs", GetShieldDuration(playerUpgrades.shieldLvl), GetShieldDuration(playerUpgrades.shieldLvl + 1));
                    DrawUpgradeRow(btnShield, "Shield Dur.", shieldTxt, GetStandardUpgradeCost(playerUpgrades.shieldLvl), shieldMax);

                    bool mushHpMax = playerUpgrades.hpMushroomLvl >= 9;
                    std::string mushHpTxt = mushHpMax 
                        ? TextFormat("+%d HP", GetMushroomHpBonus(playerUpgrades.hpMushroomLvl))
                        : TextFormat("+%d -> +%d", GetMushroomHpBonus(playerUpgrades.hpMushroomLvl), GetMushroomHpBonus(playerUpgrades.hpMushroomLvl + 1));
                    DrawUpgradeRow(btnMushHp, "Mush HP", mushHpTxt, GetStandardUpgradeCost(playerUpgrades.hpMushroomLvl), mushHpMax);

                    bool mushKillMax = playerUpgrades.killMushroomLvl >= 2;
                    std::string mushKillTxt = mushKillMax 
                        ? TextFormat("%d Cacti", GetMushroomCactusKills(playerUpgrades.killMushroomLvl))
                        : TextFormat("%d -> %d", GetMushroomCactusKills(playerUpgrades.killMushroomLvl), GetMushroomCactusKills(playerUpgrades.killMushroomLvl + 1));
                    DrawUpgradeRow(btnMushKill, "Mush Kills", mushKillTxt, GetKillMushroomCost(playerUpgrades.killMushroomLvl), mushKillMax);

                    bool spawnMax = playerUpgrades.mushSpawnRateLvl >= 8;
                    std::string spawnTxt = spawnMax 
                        ? TextFormat("%d%%", GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl))
                        : TextFormat("%d%% -> %d%%", GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl), GetMushroomSpawnChance(playerUpgrades.mushSpawnRateLvl + 1));
                    DrawUpgradeRow(btnSpawnRate, "Mush Spawn", spawnTxt, GetStandardUpgradeCost(playerUpgrades.mushSpawnRateLvl), spawnMax);

                    bool knifeMax = playerUpgrades.knifeLvl >= 1;
                    std::string knifeTxt = knifeMax ? "Knives" : "1 -> 2";
                    DrawUpgradeRow(btnKnives, "Knives", knifeTxt, 5000, knifeMax);

                    break;
                }

                case SCENE_GAME: 
                {
                    DrawBackgroundWithOverlay(g_Assets.bgOverlayGame);

                    DrawGameScene();

                    if (gameState != STATE_GAME_OVER) {
                        DrawRectangleRec(settingsBtn, DARKGRAY);
                        DrawTextEx(g_Assets.mainFont, "*", Vector2{ settingsBtn.x + 10.0f, settingsBtn.y + 5.0f }, 26.0f, 1.0f, WHITE);
                    }

                    if (showSettings) {
                        DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.85f));
                        DrawRectangleRec(backBtn, DARKGRAY);
                        DrawTextEx(g_Assets.mainFont, "< BACK", Vector2{ backBtn.x + 8.0f, backBtn.y + 7.0f }, 14.0f, 1.0f, WHITE);
                        DrawTextEx(g_Assets.mainFont, "SETTINGS", Vector2{ 120.0f, 20.0f }, 20.0f, 1.0f, WHITE);

                        DrawTextEx(g_Assets.mainFont, TextFormat("Volume: %d%%", (int)(volume * 100)), Vector2{ 50.0f, 280.0f }, 15.0f, 1.0f, WHITE);
                        DrawRectangleRec(volBar, LIGHTGRAY);
                        DrawRectangle(volBar.x, volBar.y, volBar.width * volume, volBar.height, BLUE);
                        DrawRectangleLinesEx(volBar, 2, WHITE);
                    }
                    else if (isPaused) {
                        DrawRectangle(0, 0, gameWidth, gameHeight, Fade(BLACK, 0.6f));
                        DrawTextEx(g_Assets.mainFont, "PAUSED", Vector2{ 120.0f, 140.0f }, 26.0f, 1.0f, WHITE);

                        DrawRectangleRec(proceedBtn, DARKBLUE);
                        DrawTextEx(g_Assets.mainFont, "PROCEED", Vector2{ proceedBtn.x + 30.0f, proceedBtn.y + 14.0f }, 16.0f, 1.0f, WHITE);

                        DrawRectangleRec(pauseSettingsBtn, DARKGRAY);
                        DrawTextEx(g_Assets.mainFont, "SETTINGS", Vector2{ pauseSettingsBtn.x + 25.0f, pauseSettingsBtn.y + 14.0f }, 16.0f, 1.0f, WHITE);

                        DrawRectangleRec(exitBtn, RED);
                        DrawTextEx(g_Assets.mainFont, "EXIT TO MENU", Vector2{ exitBtn.x + 12.0f, exitBtn.y + 14.0f }, 15.0f, 1.0f, WHITE);
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

    UnloadAllGameAssets();
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
