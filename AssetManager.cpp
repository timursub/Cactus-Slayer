#include "AssetManager.hpp"

GameAssets g_Assets;

void LoadAllGameAssets() {
    // 1. Background
    g_Assets.bgMain = LoadTexture("Assets/Screens/Base_Background.png");
    SetTextureFilter(g_Assets.bgMain, TEXTURE_FILTER_POINT);

    g_Assets.bgOverlayMenu = LoadTexture("Assets/Screens/Game_screen/Main_menue_screen.png");
    g_Assets.bgOverlayShop = LoadTexture("Assets/Screens/Game_screen/Upgrades_screen.png");
    g_Assets.bgOverlayGame = LoadTexture("Assets/Screens/Game_screen/Game_UI.png");

    // 2. Base Idle GIF
    g_Assets.playerIdle.LoadGif("Assets/Ninja/Ninja.gif", 8);

    // Inside LoadAllGameAssets() in AssetManager.cpp:
    if (FileExists("Assets/Ninja/Ninja_roll.gif")) {
        g_Assets.playerMoveRight.LoadGif("Assets/Ninja/Ninja_roll.gif", 4);
        g_Assets.playerMoveLeft = g_Assets.playerMoveRight;
        g_Assets.playerMoveUp = g_Assets.playerMoveRight;
        g_Assets.playerMoveDown = g_Assets.playerMoveRight;
    } else {
        g_Assets.playerMoveRight = g_Assets.playerIdle;
        g_Assets.playerMoveLeft = g_Assets.playerIdle;
        g_Assets.playerMoveUp = g_Assets.playerIdle;
        g_Assets.playerMoveDown = g_Assets.playerIdle;
    }
}

void UpdateAssetAnimations(float dt) {
    // 1. Advance frame counters (Update takes no arguments)
    g_Assets.playerIdle.Update();
    g_Assets.playerMoveRight.Update();

    // 2. Countdown active state timer using dt
    if (g_Assets.animStateTimer > 0.0f) {
        g_Assets.animStateTimer -= dt;
        if (g_Assets.animStateTimer <= 0.0f) {
            g_Assets.currentHeroState = PLAYER_ANIM_IDLE;
        }
    }
}

void SetPlayerAnimationState(PlayerAnimState newState, float duration) {
    // If we are ALREADY moving in any direction, don't reset the animation frame timer back to 0!
    bool isCurrentlyMoving = (g_Assets.currentHeroState == PLAYER_ANIM_MOVE_RIGHT ||
                              g_Assets.currentHeroState == PLAYER_ANIM_MOVE_LEFT  ||
                              g_Assets.currentHeroState == PLAYER_ANIM_MOVE_UP    ||
                              g_Assets.currentHeroState == PLAYER_ANIM_MOVE_DOWN);

    bool isNewStateMoving  = (newState == PLAYER_ANIM_MOVE_RIGHT ||
                              newState == PLAYER_ANIM_MOVE_LEFT  ||
                              newState == PLAYER_ANIM_MOVE_UP    ||
                              newState == PLAYER_ANIM_MOVE_DOWN);

    // Only switch state and refresh duration if state actually changed, or if transition wasn't movement-to-movement
    if (g_Assets.currentHeroState != newState) {
        g_Assets.currentHeroState = newState;
        g_Assets.animStateTimer = duration;
    } else {
        // Maintain active timer without rewinding GIF frame progress
        if (duration > g_Assets.animStateTimer) {
            g_Assets.animStateTimer = duration;
        }
    }
}

    // Unload all loaded textures...
void UnloadAllGameAssets() {

    UnloadTexture(g_Assets.bgMain);

    UnloadTexture(g_Assets.bgOverlayMenu);
    UnloadTexture(g_Assets.bgOverlayShop);
    UnloadTexture(g_Assets.bgOverlayGame);

    g_Assets.playerIdle.Unload();
    g_Assets.playerMoveRight.Unload();
    g_Assets.playerMoveLeft.Unload();

}