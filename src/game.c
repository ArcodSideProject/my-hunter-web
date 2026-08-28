#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define BARREL_W 60   // approximate on-screen size (3x scale of a ~20px sprite,
#define BARREL_H 60   // matching the original's `sfVector2f barrel_scale = {3,3}`)
#define GRAGAS_W 100  // 2x scale of ~50px squat sprite
#define GRAGAS_H 88

// ---- vector helpers (mirrors v2f_operations.c) ----
static Vector2 V2Mul(Vector2 v, float s) { return (Vector2){v.x * s, v.y * s}; }
static Vector2 V2Add(Vector2 a, Vector2 b) { return (Vector2){a.x + b.x, a.y + b.y}; }
static float AbsF(float x) { return x < 0 ? -x : x; }

static Rectangle BarrelRect(Barrel *b) {
    return (Rectangle){b->pos.x, b->pos.y, BARREL_W, BARREL_H};
}
static Rectangle GragasRect(Gragas *g) {
    return (Rectangle){g->pos.x, g->pos.y, GRAGAS_W, GRAGAS_H};
}

// ---- round table (mirrors easy_rounds.c) ----
// returns 1 if this round is complete (should advance), else 0
static int RoundSettings(int round, float *spawnRate, int *barrelHealth,
                          int barrelsSpawned) {
    switch (round) {
        case 1:
            *spawnRate = 0.5f; *barrelHealth = 1;
            return barrelsSpawned > 5;
        case 2:
            *spawnRate = 1.0f; *barrelHealth = 1;
            return barrelsSpawned > 15;
        case 3:
            *spawnRate = 0.5f; *barrelHealth = 3;
            return barrelsSpawned > 30;
        case 4:
            *spawnRate = 0.4f; *barrelHealth = 10;
            return barrelsSpawned > 33;
        case 5:
            *spawnRate = 6.0f; *barrelHealth = 1 + GetRandomValue(0, 1);
            return 0; // round 5 end handled separately (barrelsSpawned>70 && count==0)
        default:
            *spawnRate = 1.0f; *barrelHealth = 1;
            return 0;
    }
}

void InitGameWorld(GameWorld *world) {
    memset(world, 0, sizeof(GameWorld));
    world->state = STATE_MENU;
    world->round = 0;
    world->cloudsSpeed = 20.0f;
}

static void SpawnGragas(GameWorld *world) {
    Gragas *g = &world->gragas;
    g->exists = true;
    g->spawning = true;
    g->jumping = false;
    g->atFloor = false;
    g->acceleration = (Vector2){0, GRAVITY * 2};
    g->velocity = (Vector2){0, 250};
    g->pos = (Vector2){WIDTH / 3.0f, -100};
    g->spawnAnimation = 0;
    g->score = 0;
}

static Barrel *FreeBarrelSlot(GameWorld *world) {
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (!world->barrels[i].active) return &world->barrels[i];
    }
    return NULL;
}

static void SpawnBarrel(GameWorld *world, int maxHealth) {
    Barrel *b = FreeBarrelSlot(world);
    if (!b) return;
    memset(b, 0, sizeof(Barrel));
    b->active = true;
    b->spawning = true;
    b->pos = (Vector2){(float)GetRandomValue(0, WIDTH), HEIGHT + 50.0f};
    b->acceleration = (Vector2){0, GRAVITY};
    b->velocity = (Vector2){(float)(GetRandomValue(0, 100) - 50), -110};
    b->maxHealth = maxHealth;
    b->health = maxHealth;
    b->dead = false;
    world->barrelCount++;
    world->barrelsSpawned++;
}

// ---- barrels (mirrors barrels.c / bounce.c) ----
static void BounceOnBorder(Barrel *b) {
    float floorPos = HEIGHT - BARREL_H - FLOOR_HEIGHT;
    if (b->pos.x < 0) b->velocity.x = AbsF(b->velocity.x);
    if (b->pos.x > WIDTH - BARREL_W) b->velocity.x = -AbsF(b->velocity.x);
    if (b->spawning) return;
    b->atFloor = false;
    if (b->pos.y > floorPos) {
        if (b->velocity.y <= GRAVITY && b->velocity.y > 0) {
            b->atFloor = true;
            b->velocity.y = 0;
            return;
        }
        b->velocity = V2Mul(b->velocity, FLOOR_FRICTION);
        b->velocity.y = -AbsF(b->velocity.y);
    }
}

static void AnimateFlyingBarrel(Barrel *b, float dt) {
    b->velocity = V2Mul(b->velocity, AIR_FRICTION);
    Vector2 move = V2Mul(b->acceleration, dt);
    b->velocity = V2Add(b->velocity, move);
    BounceOnBorder(b);
    b->pos = V2Add(b->pos, V2Mul(b->velocity, dt));
}

static void AnimateBarrel(Barrel *b, float dt) {
    b->spawnTimer += dt;
    if (b->spawnTimer > 0.5f) b->spawning = false;
    b->rotation += b->velocity.x / 2.0f;
    AnimateFlyingBarrel(b, dt);
}

#define EXPLOSION_FRAME_TIME 0.035f
#define EXPLOSION_FRAMES 8

static void AnimateExplosion(Barrel *b, float dt) {
    b->explosionTimer += dt;
    if (b->explosionTimer > EXPLOSION_FRAME_TIME) {
        b->explosionFrame++;
        b->explosionTimer = 0;
        if (b->explosionFrame >= EXPLOSION_FRAMES) {
            b->active = false; // done exploding, free the slot
        }
    }
}

static void UpdateBarrels(GameWorld *world, float dt) {
    int aliveCount = 0;
    for (int i = 0; i < MAX_BARRELS; i++) {
        Barrel *b = &world->barrels[i];
        if (!b->active) continue;
        if (b->dead) {
            AnimateExplosion(b, dt);
        } else {
            AnimateBarrel(b, dt);
        }
        if (b->active) aliveCount++;
    }
    world->barrelCount = aliveCount;
}

// ---- gragas (mirrors gragas.c / goto_barrel.c / animate_spawn.c) ----
static Barrel *NearestLiveFloorBarrel(GameWorld *world) {
    // mirrors goto_barrel's linked-list walk: first non-dead barrel that's
    // landed on the floor (skips flying/dead ones)
    for (int i = 0; i < MAX_BARRELS; i++) {
        Barrel *b = &world->barrels[i];
        if (b->active && !b->dead && b->atFloor) return b;
    }
    return NULL;
}

static void GotoBarrel(Gragas *g, GameWorld *world, float dt) {
    g->acceleration.x = 0;
    Barrel *target = NearestLiveFloorBarrel(world);
    if (!target) return;

    g->walkAnimTimer += dt;
    (void)g->walkAnimTimer; // visual-only in the port; kept for parity/logging

    if (g->pos.x < target->pos.x) g->acceleration.x = 20;
    if (g->pos.x > target->pos.x) g->acceleration.x -= 20;

    // touching_barrel: same rounding-to-nearest-100 comparison as the
    // original's my_round(), reimplemented directly as a small-tolerance
    // proximity check (the original rounds screen-space X to the nearest
    // 100px bucket before comparing, which in practice just means "close
    // enough on the X axis" -- reproduced here without the float/int
    // rounding quirks of the CSFML version).
    if (AbsF(g->pos.x - target->pos.x) < 50.0f) {
        if (!target->dead) {
            target->dead = true;
            target->explosionFrame = 0;
            target->explosionTimer = 0;
            g->score++;
        }
        g->acceleration.x = 0;
    }
}

static void StandOnFloor(Gragas *g) {
    if (g->pos.y > HEIGHT - GRAGAS_H - G_FLOOR_HEIGHT) {
        g->atFloor = true;
        if (g->jumping) return;
        if (g->spawning) {
            g->spawnAnimation = 1;
            g->spawnAnimTimer = 0;
            g->spawning = false;
        }
        g->velocity.y = 0;
        g->pos.y = HEIGHT - G_FLOOR_HEIGHT - GRAGAS_H;
    }
}

static void GragasMovement(Gragas *g, GameWorld *world, float dt) {
    if (!g->spawning && !g->jumping) GotoBarrel(g, world, dt);

    g->velocity = V2Mul(g->velocity, GRAGAS_FRICTION);
    Vector2 move = V2Mul(g->acceleration, dt);
    g->velocity = V2Add(g->velocity, move);
    if (g->pos.x > WIDTH) g->velocity.x = -1;

    StandOnFloor(g);
    g->pos = V2Add(g->pos, V2Mul(g->velocity, dt));
}

static void AnimateGragasSpawn(Gragas *g, float dt) {
    g->spawnAnimTimer += dt;
    if (g->spawnAnimation == 3 && g->spawnAnimTimer > 0.50f) {
        g->spawnAnimation = 0;
    } else if (g->spawnAnimation == 2 && g->spawnAnimTimer > 0.30f) {
        g->spawnAnimTimer = 0;
        g->spawnAnimation++;
    } else if (g->spawnAnimation == 1 && g->spawnAnimTimer > 0.07f) {
        g->spawnAnimTimer = 0;
        g->spawnAnimation++;
    }
}

static void JumpingGragas(Gragas *g) {
    if (g->jumping && g->atFloor) g->jumping = false;
}

static void AnimateGragas(GameWorld *world, float dt) {
    Gragas *g = &world->gragas;
    if (!g->exists) return;
    JumpingGragas(g);
    if (g->spawnAnimation) {
        AnimateGragasSpawn(g, dt);
    } else {
        GragasMovement(g, world, dt);
    }
}

// ---- input handling (mirrors mouse.c / event_handler.c) ----
static void BarrelTouched(Barrel *b) {
    b->velocity.y = -60;
    b->velocity.x = 15.0f * ((GetRandomValue(0, 1) == 0) ? 1.0f : -1.0f);
    b->health--;
    if (b->health <= 0) {
        b->dead = true;
        b->explosionFrame = 0;
        b->explosionTimer = 0;
    }
}

static void HandleClick(GameWorld *world) {
    Vector2 m = world->mousePos;

    if (world->state == STATE_MENU) {
        // "start button" region -- centered box, matches menu draw below
        Rectangle startBtn = {WIDTH / 2.0f - 100, HEIGHT / 2.0f - 30, 200, 60};
        if (CheckCollisionPointRec(m, startBtn)) {
            world->round = 1;
            world->state = STATE_PLAYING;
        }
        return;
    }

    for (int i = 0; i < MAX_BARRELS; i++) {
        Barrel *b = &world->barrels[i];
        if (b->active && !b->dead && CheckCollisionPointRec(m, BarrelRect(b))) {
            BarrelTouched(b);
        }
    }

    Gragas *g = &world->gragas;
    if (g->exists && !g->spawning && !g->spawnAnimation &&
        CheckCollisionPointRec(m, GragasRect(g))) {
        g->velocity = (Vector2){20, -80};
        if (!g->jumping) {
            g->acceleration.x = 0;
            g->jumping = true;
            g->atFloor = false;
            g->pos.y -= 40;
        }
    }
}

// ---- round/spawn driver (mirrors rounds.c) ----
static void SpawnRoundTick(GameWorld *world, float dt) {
    world->roundClock += dt;
    float spawnRate = 1.0f;
    int barrelHealth = 1;

    if (world->round == 5) {
        RoundSettings(5, &spawnRate, &barrelHealth, world->barrelsSpawned);
        if (world->barrelsSpawned > 70 && world->barrelCount == 0) {
            world->state = STATE_GAME_OVER;
            return;
        }
    } else if (world->round >= 1 && world->round <= 4) {
        int advance = RoundSettings(world->round, &spawnRate, &barrelHealth,
                                     world->barrelsSpawned);
        if (advance) world->round++;
    }

    if (world->round == 2 && !world->gragas.exists) {
        SpawnGragas(world);
    }
    if (world->gragas.exists) {
        spawnRate -= world->gragas.score / 30.0f;
        if (spawnRate < 0.3f) spawnRate = 0.3f;
    }
    if (world->roundClock > 1.0f / spawnRate && world->barrelsSpawned < 70) {
        SpawnBarrel(world, barrelHealth);
        world->roundClock = 0;
    }
}

void UpdateGameWorld(GameWorld *world, float dt) {
    world->mousePos = GetMousePosition();
    world->cloudsX += world->cloudsSpeed * dt;
    if (world->cloudsX > WIDTH * 2) world->cloudsX = 0;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) HandleClick(world);

    if (world->state == STATE_MENU) return;

    if (world->state == STATE_GAME_OVER) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            InitGameWorld(world);
        }
        return;
    }

    SpawnRoundTick(world, dt);
    UpdateBarrels(world, dt);
    AnimateGragas(world, dt);
}

// ---- drawing ----
static void DrawBackground(GameWorld *world) {
    ClearBackground((Color){135, 206, 235, 255}); // sky
    // clouds (simple scrolling ellipses standing in for the sprite)
    for (int i = 0; i < 4; i++) {
        float x = fmodf(world->cloudsX + i * 320.0f, WIDTH + 200) - 100;
        DrawEllipse((int)x, 90 + (i % 2) * 40, 60, 24, (Color){255, 255, 255, 200});
    }
    // distant mountains
    DrawTriangle((Vector2){0, HEIGHT - FLOOR_HEIGHT - 220},
                 (Vector2){WIDTH * 0.3f, HEIGHT - FLOOR_HEIGHT - 60},
                 (Vector2){WIDTH * 0.6f, HEIGHT - FLOOR_HEIGHT - 220},
                 (Color){120, 130, 150, 255});
    // ground
    DrawRectangle(0, HEIGHT - FLOOR_HEIGHT, WIDTH, FLOOR_HEIGHT, (Color){90, 140, 60, 255});
}

static void DrawBarrel(Barrel *b) {
    Rectangle r = BarrelRect(b);
    Vector2 center = {r.x + r.width / 2, r.y + r.height / 2};

    if (b->dead) {
        float t = (float)b->explosionFrame / EXPLOSION_FRAMES;
        Color c = Fade(ORANGE, 1.0f - t);
        DrawCircleV(center, r.width / 2 * (1.0f + t), c);
        return;
    }

    unsigned char healthTint = (unsigned char)(255 * b->health / (b->maxHealth > 0 ? b->maxHealth : 1));
    Color c = b->spawning ? (Color){120, 120, 120, 220}
                           : (Color){255, healthTint, healthTint, 255};

    DrawCircleV((Vector2){center.x, r.y + r.height - 6}, r.width / 2.2f, (Color){0, 0, 0, 60}); // shadow
    Rectangle dst = {center.x, center.y, r.width, r.height};
    Vector2 origin = {r.width / 2, r.height / 2};
    // simple rotated rounded rect standing in for the barrel sprite
    DrawRectanglePro(dst, origin, b->rotation, c);
    DrawRectangleLinesEx((Rectangle){r.x, r.y, r.width, r.height}, 2, (Color){80, 50, 20, 255});
}

static void DrawGragas(Gragas *g) {
    if (!g->exists) return;
    Rectangle r = GragasRect(g);
    Color body = ORANGE;
    if (g->spawning || g->spawnAnimation) body = Fade(ORANGE, 0.6f);

    DrawEllipse((int)(r.x + r.width / 2), (int)(HEIGHT - G_FLOOR_HEIGHT - 4),
                (int)(r.width / 2.5f), 8, (Color){0, 0, 0, 60}); // shadow
    DrawRectangleRounded(r, 0.3f, 8, body);
    DrawText("G", (int)(r.x + r.width / 2 - 8), (int)(r.y + r.height / 2 - 12), 24, WHITE);
}

static void DrawHUD(GameWorld *world) {
    DrawText(TextFormat("Round: %d/5", world->round), 20, 15, 22, BLACK);
    DrawText(TextFormat("Barrels spawned: %d/70", world->barrelsSpawned), 20, 42, 18, DARKGRAY);
    if (world->gragas.exists) {
        DrawText(TextFormat("Gragas's score: %d", world->gragas.score), 20, 66, 20, MAROON);
    }
}

void DrawGameWorld(GameWorld *world) {
    DrawBackground(world);

    if (world->state == STATE_MENU) {
        const char *title = "MY HUNTER";
        int tw = MeasureText(title, 56);
        DrawText(title, WIDTH / 2 - tw / 2, HEIGHT / 2 - 140, 56, (Color){60, 30, 10, 255});

        Rectangle startBtn = {WIDTH / 2.0f - 100, HEIGHT / 2.0f - 30, 200, 60};
        DrawRectangleRounded(startBtn, 0.3f, 8, (Color){200, 80, 40, 255});
        const char *label = "START";
        int lw = MeasureText(label, 28);
        DrawText(label, (int)(startBtn.x + startBtn.width / 2 - lw / 2),
                  (int)(startBtn.y + startBtn.height / 2 - 14), 28, WHITE);
        return;
    }

    for (int i = 0; i < MAX_BARRELS; i++) {
        if (world->barrels[i].active) DrawBarrel(&world->barrels[i]);
    }
    DrawGragas(&world->gragas);
    DrawHUD(world);

    if (world->state == STATE_GAME_OVER) {
        DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 150});
        const char *title = "CLEARED!";
        int tw = MeasureText(title, 48);
        DrawText(title, WIDTH / 2 - tw / 2, HEIGHT / 2 - 60, 48, GOLD);
        const char *sub = TextFormat("Gragas's final score: %d", world->gragas.score);
        int sw = MeasureText(sub, 22);
        DrawText(sub, WIDTH / 2 - sw / 2, HEIGHT / 2, 22, RAYWHITE);
        const char *hint = "Press SPACE to play again";
        int hw = MeasureText(hint, 18);
        DrawText(hint, WIDTH / 2 - hw / 2, HEIGHT / 2 + 40, 18, LIGHTGRAY);
    }
}
