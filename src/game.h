#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "box2d/box2d.h"

// Constants mirrored from the original CSFML MyHunter's my.h
#define WIDTH 1152
#define HEIGHT 672

#define GRAVITY 5.0f
#define AIR_FRICTION 0.98f
#define FLOOR_FRICTION 0.40f
#define GRAGAS_FRICTION 0.80f

#define FLOOR_HEIGHT 55
#define G_FLOOR_HEIGHT 80

// The original scales its delta-time by GAME_TICK before using it anywhere
// (see get_delta_t.c: `dt = realSeconds * GAME_TICK`). Every velocity/
// acceleration constant in this file (GRAVITY, barrel/Gragas speeds, etc.)
// was tuned against that scaled dt, not real seconds -- so this port must
// apply the same multiplier, or everything ends up exactly GAME_TICK times
// slower than intended.
#define GAME_TICK 20.0f

#define MAX_BARRELS 800

// Box2D integration: the original's hand-rolled gravity/bounce/friction
// code had no barrel-vs-barrel collision at all, so overlapping barrels
// would visually sink into / pass through each other instead of settling
// side by side ("not fixed to the ground"). Real physics now handles
// gravity, floor contact, walls, and barrel-vs-barrel collision.
// Box2D works in meters (Y-up); the game logic stays in pixels (Y-down).
// All conversion happens at the Barrel<->b2Body boundary.
#define PIXELS_PER_METER 50.0f

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
#define TREE_BLUR 8

typedef struct {
    bool active;
    bool dead;          // popped, playing explosion animation
    bool spawning;       // brief invuln/color-fade window after spawn

    b2BodyId bodyId;      // Box2D dynamic body -- owns position/velocity while alive
    Vector2 pos;           // cached pixel-space position, refreshed from Box2D each frame
    float rotation;        // cached rotation (degrees), refreshed from Box2D each frame
    bool atFloor;           // derived each frame from Box2D velocity (settled = "at floor")

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
    Image treeImage;       // CPU-side copy of the tree texture, for pixel-perfect
                            // alpha hit-testing in the mouse-hover blur effect
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
    float treeTransparency; // 150..255, mirrors bg->tree_transparency in blur_tree.c

    float enterHoldTime;    // seconds ENTER has been held continuously
    float enterSpawnAccum;  // seconds since the last hold-triggered spawn

    Vector2 mousePos;

    Assets assets;

    b2WorldId physicsWorld;
    b2BodyId floorBody;
    b2BodyId leftWallBody;
    b2BodyId rightWallBody;
} GameWorld;

void InitGameWorld(GameWorld *world);
void InitPhysicsWorld(GameWorld *world);
void DestroyPhysicsWorld(GameWorld *world);
void LoadGameAssets(GameWorld *world);
void UnloadGameAssets(GameWorld *world);
void UpdateGameWorld(GameWorld *world, float dt);
void DrawGameWorld(GameWorld *world);

#endif
