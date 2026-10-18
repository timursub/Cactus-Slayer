#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>

// Helper wrapper for animated GIFs or Sprites
struct AnimatedSprite {
    Image animImage;
    Texture2D texture;
    int totalFrames = 0;
    int currentFrame = 0;
    int frameDelay = 0;
    int frameDelayCounter = 0;
    bool isOneShot = false;
    bool isFinished = false;

    void LoadGif(const char* filePath, int delay);
    void Update(); // <--- NO PARAMETERS
    void Draw(Vector2 centerPos, float targetWidth, Color tint = WHITE, bool flipHorizontal = false);
    void Reset();
    void Unload();
};

// Player Visual States
enum PlayerAnimState {
    PLAYER_ANIM_IDLE,
    PLAYER_ANIM_MOVE_LEFT,
    PLAYER_ANIM_MOVE_RIGHT,
    PLAYER_ANIM_MOVE_UP,
    PLAYER_ANIM_MOVE_DOWN,
    PLAYER_ANIM_DASH,
    PLAYER_ANIM_THROW
};