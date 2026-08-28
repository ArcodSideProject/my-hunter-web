#ifndef GAME_H
#define GAME_H

#include "raylib.h"

#define MAX_ENEMIES 256
#define MAX_BULLETS 256
#define MAX_PARTICLES 512
#define PLAYER_BASE_SPEED 220.0f
#define PLAYER_BASE_HP 100
#define BULLET_SPEED 520.0f
#define WORLD_WIDTH 1600
#define WORLD_HEIGHT 900

typedef enum {
    ENEMY_GRUNT = 0,
    ENEMY_RUNNER,
    ENEMY_TANK,
    ENEMY_BOSS
} EnemyType;

typedef struct {
    bool active;
    Vector2 pos;
    Vector2 vel;
    float radius;
    int hp;
    int maxHp;
    float speed;
    EnemyType type;
    float hitFlash;
} Enemy;

typedef struct {
    bool active;
    Vector2 pos;
    Vector2 vel;
    float radius;
    int damage;
    float life;
} Bullet;

typedef struct {
    bool active;
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color color;
} Particle;

typedef struct {
    Vector2 pos;
    float radius;
    int hp;
    int maxHp;
    float speed;
    float fireCooldown;
    float fireRate;
    int level;
    int xp;
    int xpToNext;
    int bulletDamage;
    float invulnTimer;
} Player;

typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_LEVEL_UP,
    STATE_GAME_OVER
} GameState;

typedef struct {
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Bullet bullets[MAX_BULLETS];
    Particle particles[MAX_PARTICLES];

    GameState state;
    int wave;
    float waveTimer;
    float spawnTimer;
    int enemiesRemainingInWave;
    int enemiesAliveCount;
    int score;
    float gameTime;

    Camera2D camera;
} GameWorld;

void InitGameWorld(GameWorld *world);
void UpdateGameWorld(GameWorld *world, float dt);
void DrawGameWorld(GameWorld *world);

#endif
