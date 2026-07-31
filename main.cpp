#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// struct to represent a position on the 3x3 grid
struct Position {
    int x;
    int y;

    // Helper operator to easily compare if two positions are equal
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

// Global list of cactuses
std::vector<Position> cactuses;

// Function to spawn a cactus on any random empty cell
void SpawnCactus(Position heroPos) {
    while (true) {
        int randX = rand() % 3; // random positions
        int randY = rand() % 3; 
        Position candidatePos = {randX, randY};

        // 1. Don't spawn near the hero
        int distToHero = std::abs(candidatePos.x - heroPos.x) + std::abs(candidatePos.y - heroPos.y);

        
        if (distToHero <= 1) continue;

        // 2. Don't spawn on top of another cactus
        bool spaceOccupied = false;
        for (const auto& cactus : cactuses) {
            if (cactus == candidatePos) {
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

// how many cactuses should be on screen based on the score
int GetCactusAmount(int score) {
    if (score >= 25000) return 3;
    if (score >= 5000)  return 2;
    return 1;
}

// check if a specific position contains a cactus
bool IsCactusAt(Position pos) {
    for (const auto& cactus : cactuses) {
        if (cactus == pos) return true;
    }
    return false;
}

enum GameState {
    STATE_PLAYING,
    STATE_GAME_OVER
};

void ResetGame(Position& hero, int& score, int& flowers, int& hp, GameState& state) {
    hero = {1, 1};
    score = 0;
    flowers = 0;
    hp = 5;
    cactuses.clear();
    SpawnCactus(hero);
    state = STATE_PLAYING;
}

int main() 
{
    
    // Seed random number generator
    srand(time(NULL));

    InitWindow(600, 1100, "Cactus Grid - Cactuses");
    SetTargetFPS(60);

    // Restart Button size
    Rectangle restartBtn = { 150, 520, 300, 90 };

    // Hero initial position (Center)
    Position hero = {1, 1};
    int score = 0;
    int highestScore = 0;
    int flowers = 0;
    int hp = 5;
    GameState gameState = STATE_PLAYING;

        

    // Spawn first Cactus
    SpawnCactus(hero);
    
        // GAME LOOP
    while (!WindowShouldClose()) 
    {
        if(gameState == STATE_PLAYING)
        {   
        
            // INPUT AND MOVEMENT
            Position nextHeroPos = hero;
            bool moved = false;
            bool isDash = false;
            bool spaceHeld = IsKeyDown(KEY_SPACE) || IsKeyPressed(KEY_SPACE);

            if (IsKeyPressed(KEY_RIGHT)) {
                // Hold Space at x=0 -> Dash all the way RIGHT to x=2
                if (spaceHeld && hero.x == 0) { nextHeroPos.x = 2; moved = true; isDash = true; }
                else if (hero.x < 2)          { nextHeroPos.x++; moved = true; }
            }
            if (IsKeyPressed(KEY_LEFT)) {
                // Hold Space at x=2 -> Dash all the way LEFT to x=0
                if (spaceHeld && hero.x == 2) { nextHeroPos.x = 0; moved = true; isDash = true; }
                else if (hero.x > 0)          { nextHeroPos.x--; moved = true; }
            }
            if (IsKeyPressed(KEY_DOWN)) {
                // Hold Space at y=0 -> Dash all the way DOWN to y=2
                if (spaceHeld && hero.y == 0) { nextHeroPos.y = 2; moved = true; isDash = true; }
                else if (hero.y < 2)          { nextHeroPos.y++; moved = true; }
            }
            if (IsKeyPressed(KEY_UP)) {
                // Hold Space at y=2 -> Dash all the way UP to y=0
                if (spaceHeld && hero.y == 2) { nextHeroPos.y = 0; moved = true; isDash = true; }
                else if (hero.y > 0)          { nextHeroPos.y--; moved = true; }
            }

            if (isDash) 
            {   
                //Calculates the bounds
                int startX = std::min(hero.x, nextHeroPos.x);
                int endX   = std::max(hero.x, nextHeroPos.x);
                int startY = std::min(hero.y, nextHeroPos.y);
                int endY   = std::max(hero.y, nextHeroPos.y);

                    //count of killed cactuses
                int killedCount = 0;

                    //kill all cactuses on the way
                for (auto it = cactuses.begin(); it != cactuses.end(); ) {
                    if (it->x >= startX && it->x <= endX && it->y >= startY && it->y <= endY) {
                        it = cactuses.erase(it);
                        killedCount++;
                    } else {
                        ++it;
                    }
                }

                    //Awards and move hero
                score += killedCount * 500;
                flowers += killedCount * 1;
                hero = nextHeroPos; // Move hero across the board

                    // Spawn new cactuses if needed
                int targetAmount = GetCactusAmount(score);
                while (cactuses.size() < targetAmount) 
                {
                    SpawnCactus(hero);
                }

                    //deal damage if nearcactus
                if (IsNearToCactus(hero)) {
                    hp--;
                }
            }
            else if (moved)
            {    // move hero if the space is NOT blocked 
                if (!IsCactusAt(nextHeroPos)) 
                {
                    hero = nextHeroPos;
                }
                else if (IsCactusAt(nextHeroPos))
                {
                    //remove cactuse
                    std::erase(cactuses, nextHeroPos); 

                    score += 500;
                    flowers += 1;
                    
                    // Spawn new cactuses if needed
                int targetAmount = GetCactusAmount(score);
                while (cactuses.size() < targetAmount) 
                {
                    SpawnCactus(hero);
                }

                }

                    if (moved &&IsNearToCactus(hero)) 
                    {
                        hp--;
                        
                    }
            }


            //convert flowers to hp
            if(flowers >= 3)
            {
                hp++;
                score = score + 250;
                flowers = flowers - 3;
            }
        
            // Ensure the number of cactuses on screen matches the required amount based on score
            while (cactuses.size() < GetCactusAmount(score)) 
            {
                SpawnCactus(hero);
            }

            // end the game
            if (hp <= 0) 
            {
                gameState = STATE_GAME_OVER;
            }
    }
    // --- GAME OVER INPUT ---
    else if (gameState == STATE_GAME_OVER) 
    {
        Vector2 mousePoint = GetMousePosition();
        
        // Check if mouse clicked inside the Restart button rectangle
        if (CheckCollisionPointRec(mousePoint, restartBtn)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                ResetGame(hero, score, flowers, hp, gameState);
            }
        }
    }

        // RENDER
        BeginDrawing();
            ClearBackground(GRAY);

            
                // Render stats text
                DrawText(TextFormat("HP: %d", hp), 400, 110, 36, RED);
                DrawText(TextFormat("SCORE: %d", score), 220, 40, 36, BLACK);
                DrawText(TextFormat("FLOWERS: %d", flowers), 40, 110, 36, DARKGREEN);

                // 1. Draw 3x3 Grid
                for (int r = 0; r < 3; r++) {
                    for (int c = 0; c < 3; c++) {
                        DrawRectangleLines((c * 180)+30, (r * 280) + 200, 180, 280, DARKGRAY);
                    }
            }

            // 2. Draw Cactuses 
            for (const auto& cactus : cactuses) {
                int cactusPixelX = cactus.x * 180 + 70;  // Centered inside 180 width
                int cactusPixelY = cactus.y * 280 + 275;  // Centered inside 280 height
                DrawRectangle(cactusPixelX, cactusPixelY, 100, 150, DARKGREEN);
            }

            // 3. Draw Hero 
            int heroPixelX = hero.x * 180 + 125;
            int heroPixelY = hero.y * 280 + 350;
            DrawCircle(heroPixelX, heroPixelY, 40, BLUE);

            // 4. Draw Game Over Screen & Restart Button
            if (gameState == STATE_GAME_OVER) {
                // Darken the background
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));

                // Game Over Title
                DrawText("GAME OVER", 130, 420, 50, RED);

                // Highlight button color when hovering with mouse
                Vector2 mousePoint = GetMousePosition();
                Color btnColor = DARKGRAY;
                if (CheckCollisionPointRec(mousePoint, restartBtn)) {
                    btnColor = LIGHTGRAY;
                }

                // Draw button shape & border
                DrawRectangleRec(restartBtn, btnColor);
                DrawRectangleLinesEx(restartBtn, 4, WHITE);
                
                // Draw button text
                DrawText("RESTART", restartBtn.x + 40, restartBtn.y + 25, 40, WHITE);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}