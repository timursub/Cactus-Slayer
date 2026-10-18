#pragma once
#include "raylib.h"
#include "Animation.hpp"

struct GameAssets {
    // Backgrounds & Screens
    Texture2D bgMain;
    
    // Fonts
    Font mainFont;
    
    // Objects
    Texture2D cactusTexture;
    Texture2D mushHpTexture;
    Texture2D mushShieldTexture;
    Texture2D mushKillTexture;

    //screenOverlays
    Texture2D bgOverlayMenu;
    Texture2D bgOverlayShop;
    Texture2D bgOverlayGame;

    // Player Animations Map
    AnimatedSprite playerIdle;
    AnimatedSprite playerMoveLeft;
    AnimatedSprite playerMoveRight;
    AnimatedSprite playerMoveUp;
    AnimatedSprite playerMoveDown;
    AnimatedSprite playerDash;

    // Animation Timer state
    PlayerAnimState currentHeroState = PLAYER_ANIM_IDLE;
    float animStateTimer = 0.0f;
};

// Global Asset Instance Accessor
extern GameAssets g_Assets;

void LoadAllGameAssets();
void UnloadAllGameAssets();
void UpdateAssetAnimations(float dt);
void SetPlayerAnimationState(PlayerAnimState newState, float duration = 0.0f);