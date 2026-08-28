#ifndef GAME_H
#define GAME_H

#include "raylib.h"

// Constants mirrored from the original CSFML MyHunter's my.h
#define WIDTH 1152
#define HEIGHT 672

#define GRAVITY 5.0f
#define AIR_FRICTION 0.98f
#define FLOOR_FRICTION 0.40f
#define GRAGAS_FRICTION 0.80f

#define FLOOR_HEIGHT 55
#define G_FLOOR_HEIGHT 80

#define MAX_BARRELS 128

// Sprite-sheet frame layout, mirrored exactly from my.h
#define EXPLOSION_WIDTH 50
#define EXPLOSION_HEIGHT 51
#define SQUATTING_GRAGAS_WIDTH 50
#define SQUATTING_GRAGAS_HEIGHT 44
#define STANDING_GRAGAS_WIDTH 66
#define STANDING_GRAGAS_HEIGHT 62
#define STANDING_GRAGAS_HEIGHT_OFFSET 46

#define TREE_OFFSET_X 173
#define TREE_OFFSET_Y 82

typedef struct {
    bool active;
    bool dead;          // popped, playing explosion animation
    bool spawning;       // brief invuln/color-fade window after spawn
    bool atFloor;

    Vector2 pos;          // top-left, mirrors sfSprite position semantics
    Vector2 velocity;
    Vector2 acceleration;
    float rotation;

    int maxHealth;
    int health;

    float explosionTimer;
    int explosionFrame;   // 0..~N, drives explosion sprite-sheet style fade

    float spawnTimer;      // used for the 0.5s "still fading in" window
} Barrel;

typedef enum {
    G_IDLE = 0,
    G_SPAWNING,
    G_JUMPING
} GragasAnimState;

typedef struct {
    bool exists;
    Vector2 pos;
    Vector2 velocity;
    Vector2 acceleration;
    bool jumping;
    bool atFloor;
    bool spawning;
    int spawnAnimation;  // 0..3, mirrors the original's spawn animation steps
    float spawnAnimTimer;
    float walkAnimTimer;
    int walkFrame;        // 0..2, mirrors rect_anim.left stepping through 3 squat frames
    int score;
} Gragas;

typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

typedef struct {
    Texture2D barrel;
    Texture2D sumos;       // Gragas sprite sheet (squatting + standing frames)
    Texture2D explosion;
    Texture2D startButton;
    Texture2D sky, clouds, mountain, farWoods, tiles, tree, frontGrass;
    Font font;
    bool loaded;
} Assets;

typedef struct {
    GameState state;
    Barrel barrels[MAX_BARRELS];
    int barrelCount;      // currently alive+exploding barrels
    int barrelsSpawned;   // total ever spawned this game

    Gragas gragas;

    int round;             // 1..5
    float roundClock;      // seconds since last spawn-rate reset condition
    float spawnClock;      // seconds since last barrel spawn

    float cloudsX;
    float cloudsSpeed;

    Vector2 mousePos;

    Assets assets;
} GameWorld;

void InitGameWorld(GameWorld *world);
void LoadGameAssets(GameWorld *world);
void UnloadGameAssets(GameWorld *world);
void UpdateGameWorld(GameWorld *world, float dt);
void DrawGameWorld(GameWorld *world);

#endif
