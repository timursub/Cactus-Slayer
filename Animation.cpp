#include "Animation.hpp"

void AnimatedSprite::LoadGif(const char* filePath, int delay) {
    frameDelay = delay;
    animImage = LoadImageAnim(filePath, &totalFrames);
    texture = LoadTextureFromImage(animImage);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    currentFrame = 0;
    frameDelayCounter = 0;
    isFinished = false;
}

void AnimatedSprite::Update() {
    if (totalFrames <= 1 || (isOneShot && isFinished)) return;

    frameDelayCounter++;
    if (frameDelayCounter >= frameDelay) {
        frameDelayCounter = 0;
        currentFrame++;

        if (currentFrame >= totalFrames) {
            if (isOneShot) {
                currentFrame = totalFrames - 1; // Hold last frame
                isFinished = true;
            } else {
                currentFrame = 0; // Loop animation
            }
        }

        // Update GPU texture buffer with current frame offset
        unsigned int nextFrameOffset = animImage.width * animImage.height * 4 * currentFrame;
        UpdateTexture(texture, ((unsigned char*)animImage.data) + nextFrameOffset);
    }
}

// In Animation.cpp:
void AnimatedSprite::Draw(Vector2 centerPos, float targetWidth, Color tint, bool flipHorizontal) {
    float aspectRatio = (float)texture.height / (float)texture.width;
    float targetHeight = targetWidth * aspectRatio;

    // A negative width in srcRec flips the texture horizontally!
    float srcWidth = flipHorizontal ? -(float)texture.width : (float)texture.width;

    Rectangle srcRec = { 0.0f, 0.0f, srcWidth, (float)texture.height };
    Rectangle destRec = {
        centerPos.x - (targetWidth / 2.0f),
        centerPos.y - (targetHeight / 2.0f),
        targetWidth,
        targetHeight
    };

    DrawTexturePro(texture, srcRec, destRec, Vector2{ 0.0f, 0.0f }, 0.0f, tint);
}

void AnimatedSprite::Reset() {
    currentFrame = 0;
    frameDelayCounter = 0;
    isFinished = false;
}

void AnimatedSprite::Unload() {
    UnloadTexture(texture);
    UnloadImage(animImage);
}