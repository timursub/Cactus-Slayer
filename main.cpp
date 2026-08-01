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

//mashroom types
enum MushroomType {
    MSH_HP,          // +1 HP
    MSH_SHIELD,      // Shield for 15 seconds
    MSH_KILL_CACTUS  // Kills a random cactus
};

//mashroom constructor
struct Mushroom {
    Position pos;
    MushroomType type;
};

// Global vector for mushrooms on screen
std::vector<Mushroom> mushrooms;
// Global list of cactuses
std::vector<Position> cactuses;


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
    if (score >= 25000) return 3;
    if (score >= 5000)  return 2;
    return 1;
}


// game status
enum GameState {
    STATE_PLAYING,
    STATE_GAME_OVER
};


// Mashroom spawn
void SpawnMushroom(Position heroPos) {
    while (true) {
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

// reset game after the game over
void ResetGame(Position& hero, int& score, int& flowers, int& hp, float& shieldEndTime, GameState& state) {
    hero = {1, 1};
    score = 0;
    flowers = 0;
    hp = 5;
    shieldEndTime = 0.0f;
    cactuses.clear();
    mushrooms.clear();

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
    float shieldEndTime = 0; 
    //chack if we need new cactus
    bool pendingCactusRespawn = false; 


        

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


                //Respawn cactus if destroed by mushroom
            if (moved || isDash) 
            {
               
                if (pendingCactusRespawn) {
                    SpawnCactus(hero);
                    pendingCactusRespawn = false;
                }
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

                // Check if hero lands on a mushroom
                for (auto it = mushrooms.begin(); it != mushrooms.end(); ) 
                {
                    if (it->pos == hero) 
                    {
                        if (it->type == MSH_HP)
                        {
                            hp++;
                        }
                        else if (it->type == MSH_SHIELD) 
                        {
                            // Give 15 seconds of shield 
                            shieldEndTime = GetTime() + 7.0f;
                        }
                        else if (it->type == MSH_KILL_CACTUS) 
                        {
                            // Kill a random cactus if any exist
                            if (it->type == MSH_KILL_CACTUS) 
                            {
                                if (!cactuses.empty()) 
                                {
                                    int indexToKill = rand() % cactuses.size();
                                    cactuses.erase(cactuses.begin() + indexToKill);
                                    
                                    //Marks that cactus need to be respawned
                                    pendingCactusRespawn = true; 
                                }
                            }
                        }
                        it = mushrooms.erase(it); // Remove collected mushroom
                    } else 
                        {
                         ++it;
                        }
                }

                

                    //deal damage if near cactus, with no shield
                if (IsNearToCactus(hero) && !IsShieldActive(shieldEndTime)) {
                hp--;
                }


                //spawn mashroom
                if (score >= 15000) {
                    //15% chance
                    if ((rand() % 100) < 5) {
                       // one mashroom limit
                        if (mushrooms.empty()) {
                            SpawnMushroom(hero);
                        }
                    }
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
                    
                  

                }

                    if (moved && IsNearToCactus(hero) && !IsShieldActive(shieldEndTime)) 
                    {
                        hp--;
                        
                    }

                    // Check if hero lands on a mushroom
                for (auto it = mushrooms.begin(); it != mushrooms.end(); ) 
                {
                    if (it->pos == hero) 
                    {
                        if (it->type == MSH_HP)
                        {
                            hp++;
                        }
                        else if (it->type == MSH_SHIELD) 
                        {
                            // Give 15 seconds of shield 
                            shieldEndTime = GetTime() + 7.0f;
                        }
                        else if (it->type == MSH_KILL_CACTUS) 
                            {
                                if (!cactuses.empty()) 
                                {
                                    int indexToKill = rand() % cactuses.size();
                                    cactuses.erase(cactuses.begin() + indexToKill);
                                    
                                    //Marks that cactus need to be respawned
                                    pendingCactusRespawn = true; 
                                }
                            }
                        it = mushrooms.erase(it); // Remove collected mushroom
                    } else 
                        {
                         ++it;
                        }
                }

                    //spawn mashroom
                if (score >= 15000) {
                    //15% chance
                    if ((rand() % 100) < 5) {
                       // one mashroom limit
                        if (mushrooms.empty()) {
                            SpawnMushroom(hero);
                        }
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

                //ubdates highst score
            if (score > highestScore) highestScore = score;
        
                //Cakcukates how many cactuses should be on screen
            int targetCactuses = GetCactusAmount(score);

            // If a mushroom killed a cactus on this move, hold back 1 slot from spawning right now!
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
        // GAME OVER INPUT
        else if (gameState == STATE_GAME_OVER) 
        {
            Vector2 mousePoint = GetMousePosition();
            
            // Check if mouse clicked inside the Restart button rectangle
            if (CheckCollisionPointRec(mousePoint, restartBtn)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ResetGame(hero, score, flowers, hp, shieldEndTime, gameState);
                }
            }
        }

        // RENDER
        BeginDrawing();
            ClearBackground(GRAY);

            // UPPER MENUE UI
        // Top left stats
        DrawText(TextFormat("HI-SCORE: %d", highestScore), 30, 20, 28, GOLD);
        DrawText(TextFormat("SCORE: %d", score), 30, 60, 28, BLACK);
        DrawText(TextFormat("HP: %d", hp), 30, 100, 28, RED);

        // Top right - flowers
        DrawText("FLOWERS", 400, 30, 22, DARKGREEN);

            for (int i = 0; i < 3; i++) {
                int slotX = 410 + (i * 45); // Spacing flowers 45px apart horizontally
                int slotY = 85;

                // Pick color: Active if collected, Gray if empty
                Color flowerColor = (i < flowers) ? PINK : DARKGRAY;

                // Draw Flower Icon (Center circle + outer ring/petals)
                DrawCircle(slotX, slotY, 14, flowerColor);
                DrawCircleLines(slotX, slotY, 15, WHITE); // White border
            }

                //  3X3 GRID
                for (int r = 0; r < 3; r++)     
                {
                    for (int c = 0; c < 3; c++) 
                    {
                        DrawRectangleLines((c * 180)+30, (r * 280) + 200, 180, 280, DARKGRAY);
                    }
                }

            //  Draw Cactuses 
            for (const auto& cactus : cactuses) {
                int cactusPixelX = cactus.x * 180 + 70;  // Centered inside 180 width
                int cactusPixelY = cactus.y * 280 + 275;  // Centered inside 280 height
                DrawRectangle(cactusPixelX, cactusPixelY, 100, 150, DARKGREEN);
            }

            //  Draw Hero 
            int heroPixelX = hero.x * 180 + 125;
            int heroPixelY = hero.y * 280 + 350;
            DrawCircle(heroPixelX, heroPixelY, 40, BLUE);

            
                //Draw Mashrooms
            for (const auto& m : mushrooms) {
                int mPixelX = m.pos.x * 180 + 125;
                int mPixelY = m.pos.y * 280 + 350;

                Color mColor = PURPLE; // Default
                if (m.type == MSH_HP) mColor = RED;
                else if (m.type == MSH_SHIELD) mColor = SKYBLUE;
                else if (m.type == MSH_KILL_CACTUS) mColor = ORANGE;

                // Draw mushroom cap
                DrawCircle(mPixelX, mPixelY, 25, mColor);
            }

            // Optional: Draw a shield aura around the hero if active!
            if (GetTime() < shieldEndTime) {
                int heroPixelX = hero.x * 180 + 125;
                int heroPixelY = hero.y * 280 + 350;
                DrawCircleLines(heroPixelX, heroPixelY, 50, SKYBLUE);
            }

            //  Draw Game Over Screen & Restart Button
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