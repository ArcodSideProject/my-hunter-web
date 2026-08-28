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
    Assets savedAssets = world->assets; // preserve loaded textures across resets
    memset(world, 0, sizeof(GameWorld));
    world->assets = savedAssets;
    world->state = STATE_MENU;
    world->round = 0;
    world->cloudsSpeed = 20.0f;
    world->treeTransparency = 255.0f;
}

void LoadGameAssets(GameWorld *world) {
    Assets *a = &world->assets;
    a->barrel = LoadTexture("assets/barrel.png");
    a->sumos = LoadTexture("assets/sumos.png");
    a->explosion = LoadTexture("assets/explosion.png");
    a->startButton = LoadTexture("assets/start_button.png");
    a->sky = LoadTexture("assets/background/Sky.png");
    a->clouds = LoadTexture("assets/background/Clouds.png");
    a->mountain = LoadTexture("assets/background/Mountain.png");
    a->farWoods = LoadTexture("assets/background/Far_woods.png");
    a->tiles = LoadTexture("assets/background/Tiles.png");
    a->tree = LoadTexture("assets/background/Tree.png");
    a->frontGrass = LoadTexture("assets/background/Front_grass.png");
    // CPU-side copy of the tree, used for pixel-perfect alpha hit-testing
    // in the mouse-hover blur effect (matches blur_tree.c's
    // sfImage_getPixel(...).a check) -- a plain Texture2D can't be read
    // back on the CPU without this.
    a->treeImage = LoadImage("assets/background/Tree.png");
    a->font = LoadFontEx("assets/fonts/upheavtt.ttf", 48, NULL, 0);
    a->loaded = (a->barrel.id != 0 && a->sumos.id != 0);
    if (!a->loaded) {
        TraceLog(LOG_WARNING, "LoadGameAssets: one or more textures failed to load; "
                 "falling back to placeholder shapes");
    }
}

void UnloadGameAssets(GameWorld *world) {
    Assets *a = &world->assets;
    if (!a->loaded) return;
    UnloadTexture(a->barrel);
    UnloadTexture(a->sumos);
    UnloadTexture(a->explosion);
    UnloadTexture(a->startButton);
    UnloadTexture(a->sky);
    UnloadTexture(a->clouds);
    UnloadTexture(a->mountain);
    UnloadTexture(a->farWoods);
    UnloadTexture(a->tiles);
    UnloadTexture(a->tree);
    UnloadTexture(a->frontGrass);
    UnloadImage(a->treeImage);
    UnloadFont(a->font);
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
    if (!b) { TraceLog(LOG_WARNING, "SpawnBarrel: no free slot!"); return; }
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
            // Unlike the original (which only zeroes velocity and lets
            // whatever position was already reached stand, occasionally
            // sinking a few px into the floor on a fast/coarse frame),
            // explicitly clamp to the floor line here so barrels always
            // visibly rest on top of the ground instead of underneath it.
            b->pos.y = floorPos;
            return;
        }
        b->velocity = V2Mul(b->velocity, FLOOR_FRICTION);
        b->velocity.y = -AbsF(b->velocity.y);
    }
}

static void AnimateFlyingBarrel(Barrel *b, float physicsDt) {
    b->velocity = V2Mul(b->velocity, AIR_FRICTION);
    Vector2 move = V2Mul(b->acceleration, physicsDt);
    b->velocity = V2Add(b->velocity, move);
    BounceOnBorder(b);
    b->pos = V2Add(b->pos, V2Mul(b->velocity, physicsDt));
}

static void AnimateBarrel(Barrel *b, float dt, float physicsDt) {
    b->spawnTimer += dt; // real time, matches sfClock_getElapsedTime > 500000us
    if (b->spawnTimer > 0.5f) b->spawning = false;
    b->rotation += b->velocity.x / 2.0f;
    AnimateFlyingBarrel(b, physicsDt);
}

#define EXPLOSION_FRAME_TIME 0.035f
#define EXPLOSION_FRAMES 6  // real frame count in explosion.png (315px / (50+3)px stride)

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

static void UpdateBarrels(GameWorld *world, float dt, float physicsDt) {
    int aliveCount = 0;
    for (int i = 0; i < MAX_BARRELS; i++) {
        Barrel *b = &world->barrels[i];
        if (!b->active) continue;
        if (b->dead) {
            AnimateExplosion(b, dt);
        } else {
            AnimateBarrel(b, dt, physicsDt);
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

    // walking_animation: step through 3 squat frames, cadence tied to
    // horizontal speed (faster walk = faster frame cycling), same formula
    // as the original's `0.05 + 1/|velocity.x|` threshold. Uses real dt,
    // matching sfClock_getElapsedTime (unscaled).
    g->walkAnimTimer += dt;
    float threshold = 0.05f + (1.0f / (AbsF(g->velocity.x) > 0.01f ? AbsF(g->velocity.x) : 0.01f));
    if (g->walkAnimTimer > threshold) {
        g->walkFrame = (g->walkFrame + 1) % 3;
        g->walkAnimTimer = 0;
    }

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

static void GragasMovement(Gragas *g, GameWorld *world, float dt, float physicsDt) {
    if (!g->spawning && !g->jumping) GotoBarrel(g, world, dt);

    g->velocity = V2Mul(g->velocity, GRAGAS_FRICTION);
    Vector2 move = V2Mul(g->acceleration, physicsDt);
    g->velocity = V2Add(g->velocity, move);
    if (g->pos.x > WIDTH) g->velocity.x = -1;

    StandOnFloor(g);
    g->pos = V2Add(g->pos, V2Mul(g->velocity, physicsDt));
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

static void AnimateGragas(GameWorld *world, float dt, float physicsDt) {
    Gragas *g = &world->gragas;
    if (!g->exists) return;
    JumpingGragas(g);
    if (g->spawnAnimation) {
        AnimateGragasSpawn(g, dt);
    } else {
        GragasMovement(g, world, dt, physicsDt);
    }
}

// ---- input handling (mirrors mouse.c / event_handler.c) ----
static void BlurTree(GameWorld *world) {
    Assets *a = &world->assets;
    if (!a->loaded) return;
    float scaleX = WIDTH / 384.0f, scaleY = HEIGHT / 224.0f;

    // Tree on-screen bounds, matching sfSprite_getGlobalBounds(bg->tree):
    // position is (TREE_OFFSET_X*scaleX, TREE_OFFSET_Y*scaleY), size is the
    // raw Tree.png dimensions scaled by the same background scale factor.
    Rectangle treeRect = {
        TREE_OFFSET_X * scaleX, TREE_OFFSET_Y * scaleY,
        a->tree.width * scaleX, a->tree.height * scaleY
    };

    bool overOpaquePixel = false;
    if (CheckCollisionPointRec(world->mousePos, treeRect)) {
        // map screen-space mouse position back to a pixel in the original
        // (unscaled) tree image, exactly mirroring blur_tree.c's
        // `(mpos - TREE_OFFSET * resize) / 3` (the original always divides
        // by the fixed 3x background scale; we use the real scaleX/scaleY
        // here so this still lines up if the window is ever resized).
        int px = (int)((world->mousePos.x - TREE_OFFSET_X * scaleX) / scaleX);
        int py = (int)((world->mousePos.y - TREE_OFFSET_Y * scaleY) / scaleY);
        if (px >= 0 && px < a->treeImage.width && py >= 0 && py < a->treeImage.height) {
            Color c = GetImageColor(a->treeImage, px, py);
            overOpaquePixel = (c.a != 0);
        }
    }

    if (overOpaquePixel) {
        if (world->treeTransparency > 150.0f) world->treeTransparency -= TREE_BLUR;
    } else {
        if (world->treeTransparency < 255.0f - TREE_BLUR) {
            world->treeTransparency += TREE_BLUR;
        } else {
            world->treeTransparency = 255.0f;
        }
    }
}

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
    // IMPORTANT: the original only scales dt by GAME_TICK for the actual
    // physics integration (velocity/position updates in movement() /
    // animate_flying_barrel()). Everything else -- spawn_round's timer,
    // animation frame timers, explosion timers -- uses real wall-clock
    // time via sfClock_getElapsedTime(), completely unscaled. Mixing the
    // two was the original bug in this port: using scaled dt everywhere
    // made movement correct-ISH but everything crawled, because dt itself
    // was being treated as "real seconds" when constants expected
    // "real seconds * 20".
    float physicsDt = dt * GAME_TICK;

    world->mousePos = GetMousePosition();
    world->cloudsX += world->cloudsSpeed * dt;
    if (world->cloudsX > WIDTH * 2) world->cloudsX = 0;
    BlurTree(world); // runs every frame regardless of state, matching render_window()

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) HandleClick(world);

    // ENTER: manual debug barrel spawn (1 hp), matches manage_keys()'s
    // `spawn_barrel(g, 1)` on sfKeyEnter -- a real feature of the original,
    // not a test-only shortcut.
    if (world->state == STATE_PLAYING && IsKeyPressed(KEY_ENTER)) {
        SpawnBarrel(world, 1);
    }

    // SPACE: force-spawn Gragas immediately if he doesn't exist yet,
    // matches manage_keys()'s sfKeySpace branch. In the original this is
    // available any time (menu included), independent of round number --
    // it's a debug/cheat key, not a "start game" button.
    if (IsKeyPressed(KEY_SPACE) && !world->gragas.exists) {
        SpawnGragas(world);
    }

    if (world->state == STATE_MENU) return;

    if (world->state == STATE_GAME_OVER) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            InitGameWorld(world);
        }
        return;
    }

    SpawnRoundTick(world, dt);          // real-time spawn cadence
    UpdateBarrels(world, dt, physicsDt); // physics uses scaled dt, timers use real dt
    AnimateGragas(world, dt, physicsDt);
}

// ---- drawing ----
static void DrawBackground(GameWorld *world) {
    Assets *a = &world->assets;
    if (!a->loaded) {
        ClearBackground((Color){135, 206, 235, 255});
        for (int i = 0; i < 4; i++) {
            float x = fmodf(world->cloudsX + i * 320.0f, WIDTH + 200) - 100;
            DrawEllipse((int)x, 90 + (i % 2) * 40, 60, 24, (Color){255, 255, 255, 200});
        }
        DrawTriangle((Vector2){0, HEIGHT - FLOOR_HEIGHT - 220},
                     (Vector2){WIDTH * 0.3f, HEIGHT - FLOOR_HEIGHT - 60},
                     (Vector2){WIDTH * 0.6f, HEIGHT - FLOOR_HEIGHT - 220},
                     (Color){120, 130, 150, 255});
        DrawRectangle(0, HEIGHT - FLOOR_HEIGHT, WIDTH, FLOOR_HEIGHT, (Color){90, 140, 60, 255});
        return;
    }

    // Every background layer is a 384x224 source scaled up 3x to fill the
    // 1152x672 window, exactly matching create_background.c's
    // `resize = WIDTH/384, HEIGHT/224` (== 3,3).
    Rectangle dst = {0, 0, WIDTH, HEIGHT};
    DrawTexturePro(a->sky, (Rectangle){0, 0, (float)a->sky.width, (float)a->sky.height},
                    dst, (Vector2){0, 0}, 0, WHITE);

    // clouds scroll horizontally and wrap, matching move_clouds()
    float cx = fmodf(world->cloudsX, WIDTH);
    Rectangle cloudsSrc = {0, 0, (float)a->clouds.width, (float)a->clouds.height};
    DrawTexturePro(a->clouds, cloudsSrc, (Rectangle){cx, 0, WIDTH, HEIGHT}, (Vector2){0, 0}, 0, WHITE);
    DrawTexturePro(a->clouds, cloudsSrc, (Rectangle){cx - WIDTH, 0, WIDTH, HEIGHT}, (Vector2){0, 0}, 0, WHITE);

    DrawTexturePro(a->mountain, (Rectangle){0, 0, (float)a->mountain.width, (float)a->mountain.height},
                    dst, (Vector2){0, 0}, 0, WHITE);
    DrawTexturePro(a->farWoods, (Rectangle){0, 0, (float)a->farWoods.width, (float)a->farWoods.height},
                    dst, (Vector2){0, 0}, 0, WHITE);

    // tree, offset per TREE_OFFSET_X/Y * scale, matching create_background.c
    float scaleX = WIDTH / 384.0f, scaleY = HEIGHT / 224.0f;
    DrawTextureEx(a->tree, (Vector2){TREE_OFFSET_X * scaleX, TREE_OFFSET_Y * scaleY}, 0, scaleX,
                  (Color){255, 255, 255, (unsigned char)world->treeTransparency});

    DrawTexturePro(a->tiles, (Rectangle){0, 0, (float)a->tiles.width, (float)a->tiles.height},
                    dst, (Vector2){0, 0}, 0, WHITE);
    DrawTexturePro(a->frontGrass, (Rectangle){0, 0, (float)a->frontGrass.width, (float)a->frontGrass.height},
                    dst, (Vector2){0, 0}, 0, WHITE);
}

static void DrawBarrel(GameWorld *world, Barrel *b) {
    Assets *a = &world->assets;
    Rectangle r = BarrelRect(b);
    Vector2 center = {r.x + r.width / 2, r.y + r.height / 2};

    if (!a->loaded) {
        if (b->dead) {
            float t = (float)b->explosionFrame / EXPLOSION_FRAMES;
            DrawCircleV(center, r.width / 2 * (1.0f + t), Fade(ORANGE, 1.0f - t));
            return;
        }
        unsigned char healthTint = (unsigned char)(255 * b->health / (b->maxHealth > 0 ? b->maxHealth : 1));
        Color c = b->spawning ? (Color){120, 120, 120, 220} : (Color){255, healthTint, healthTint, 255};
        DrawCircleV((Vector2){center.x, r.y + r.height - 6}, r.width / 2.2f, (Color){0, 0, 0, 60});
        Rectangle dst = {center.x, center.y, r.width, r.height};
        Vector2 origin = {r.width / 2, r.height / 2};
        DrawRectanglePro(dst, origin, b->rotation, c);
        DrawRectangleLinesEx((Rectangle){r.x, r.y, r.width, r.height}, 2, (Color){80, 50, 20, 255});
        return;
    }

    // shadow, matching calculate_barrel_shadow's simple ellipse under the barrel
    DrawEllipse((int)center.x, (int)(r.y + r.height - 4), (int)(r.width / 2.2f), 6,
                (Color){0, 0, 0, 70});

    if (b->dead) {
        // explosion.png is a horizontal strip of 6 EXPLOSION_WIDTH x
        // EXPLOSION_HEIGHT frames with a 3px gap between them (stride =
        // EXPLOSION_WIDTH + 3, matching animate_explosion's
        // `rect_anim.left += EXPLOSION_WIDTH + 3` -- using a bare
        // EXPLOSION_WIDTH stride here was the earlier misalignment bug,
        // it drifts more with every frame since the sheet actually has
        // gaps between frames), stepped every ~0.035s.
        int stride = EXPLOSION_WIDTH + 3;
        int maxFrame = (a->explosion.width) / stride; // last full frame index
        int frame = b->explosionFrame;
        if (frame > maxFrame) frame = maxFrame;
        Rectangle src = {(float)(frame * stride), 0, EXPLOSION_WIDTH, EXPLOSION_HEIGHT};
        Rectangle dst = {center.x, center.y, EXPLOSION_WIDTH * 1.5f, EXPLOSION_HEIGHT * 1.5f};
        Vector2 origin = {EXPLOSION_WIDTH * 1.5f / 2, EXPLOSION_HEIGHT * 1.5f / 2};
        DrawTexturePro(a->explosion, src, dst, origin, 0, WHITE);
        return;
    }

    // health tint: white->red as health drops, matching animate_barrels'
    // sfColor_fromRGB(255, health*255/maxHealth, health*255/maxHealth)
    unsigned char healthTint = (unsigned char)(255 * b->health / (b->maxHealth > 0 ? b->maxHealth : 1));
    Color tint = b->spawning ? (Color){120, 120, 120, 220} : (Color){255, healthTint, healthTint, 255};

    Rectangle src = {0, 0, (float)a->barrel.width, (float)a->barrel.height};
    Rectangle dst = {center.x, center.y, r.width, r.height};
    Vector2 origin = {r.width / 2, r.height / 2};
    DrawTexturePro(a->barrel, src, dst, origin, b->rotation, tint);
}

static void DrawGragas(GameWorld *world) {
    Gragas *g = &world->gragas;
    if (!g->exists) return;
    Assets *a = &world->assets;
    Rectangle r = GragasRect(g);

    if (!a->loaded) {
        Color body = (g->spawning || g->spawnAnimation) ? Fade(ORANGE, 0.6f) : ORANGE;
        DrawEllipse((int)(r.x + r.width / 2), (int)(HEIGHT - G_FLOOR_HEIGHT - 4),
                    (int)(r.width / 2.5f), 8, (Color){0, 0, 0, 60});
        DrawRectangleRounded(r, 0.3f, 8, body);
        DrawText("G", (int)(r.x + r.width / 2 - 8), (int)(r.y + r.height / 2 - 12), 24, WHITE);
        return;
    }

    DrawEllipse((int)(r.x + r.width / 2), (int)(HEIGHT - G_FLOOR_HEIGHT - 4),
                (int)(r.width / 2.5f), 8, (Color){0, 0, 0, 60});

    // sumos.png layout mirrors rect_anim in the original: standing/spawn
    // frames use STANDING_GRAGAS_WIDTH/HEIGHT at a Y offset, squatting/walk
    // frames use SQUATTING_GRAGAS_WIDTH/HEIGHT starting at Y=0.
    Rectangle src;
    if (g->spawning || g->spawnAnimation) {
        int frame = g->spawnAnimation; // 0..3 walks through the spawn strip
        src = (Rectangle){(float)(frame * (STANDING_GRAGAS_WIDTH + 2)),
                            STANDING_GRAGAS_HEIGHT_OFFSET,
                            STANDING_GRAGAS_WIDTH, STANDING_GRAGAS_HEIGHT};
    } else if (g->jumping) {
        src = (Rectangle){210, 0, SQUATTING_GRAGAS_WIDTH, SQUATTING_GRAGAS_HEIGHT};
    } else {
        src = (Rectangle){(float)(g->walkFrame * (SQUATTING_GRAGAS_WIDTH + 2)), 0,
                            SQUATTING_GRAGAS_WIDTH, SQUATTING_GRAGAS_HEIGHT};
    }
    // clamp source rect inside the actual texture bounds defensively --
    // sumos.png is a community-sourced sheet and frame counts can vary
    // slightly from the original's hardcoded offsets.
    if (src.x + src.width > a->sumos.width) src.x = 0;
    if (src.y + src.height > a->sumos.height) src.y = 0;

    Rectangle dst = {r.x, r.y, r.width, r.height};
    DrawTexturePro(a->sumos, src, dst, (Vector2){0, 0}, 0, WHITE);
}

static void DrawHUD(GameWorld *world) {
    Assets *a = &world->assets;
    Font f = a->loaded ? a->font : GetFontDefault();
    DrawTextEx(f, TextFormat("Round: %d/5", world->round), (Vector2){20, 15}, 22, 1, BLACK);
    DrawTextEx(f, TextFormat("Barrels spawned: %d/70", world->barrelsSpawned), (Vector2){20, 42}, 18, 1, DARKGRAY);
    if (world->gragas.exists) {
        DrawTextEx(f, TextFormat("Gragas's score: %d", world->gragas.score), (Vector2){20, 66}, 20, 1, MAROON);
    }
}

void DrawGameWorld(GameWorld *world) {
    DrawBackground(world);
    Assets *a = &world->assets;
    Font f = a->loaded ? a->font : GetFontDefault();

    if (world->state == STATE_MENU) {
        const char *title = "MY HUNTER";
        Vector2 tsize = MeasureTextEx(f, title, 56, 1);
        DrawTextEx(f, title, (Vector2){WIDTH / 2 - tsize.x / 2, HEIGHT / 2 - 140}, 56, 1,
                   (Color){60, 30, 10, 255});

        Rectangle startBtn = {WIDTH / 2.0f - 100, HEIGHT / 2.0f - 30, 200, 60};
        if (a->loaded && a->startButton.id != 0) {
            Rectangle src = {0, 0, (float)a->startButton.width, (float)a->startButton.height};
            DrawTexturePro(a->startButton, src, startBtn, (Vector2){0, 0}, 0, WHITE);
        } else {
            DrawRectangleRounded(startBtn, 0.3f, 8, (Color){200, 80, 40, 255});
            const char *label = "START";
            Vector2 lsize = MeasureTextEx(f, label, 28, 1);
            DrawTextEx(f, label, (Vector2){startBtn.x + startBtn.width / 2 - lsize.x / 2,
                       startBtn.y + startBtn.height / 2 - 14}, 28, 1, WHITE);
        }
        return;
    }

    for (int i = 0; i < MAX_BARRELS; i++) {
        if (world->barrels[i].active) DrawBarrel(world, &world->barrels[i]);
    }
    DrawGragas(world);
    DrawHUD(world);

    if (world->state == STATE_GAME_OVER) {
        DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 150});
        const char *title = "CLEARED!";
        Vector2 tsize = MeasureTextEx(f, title, 48, 1);
        DrawTextEx(f, title, (Vector2){WIDTH / 2 - tsize.x / 2, HEIGHT / 2 - 60}, 48, 1, GOLD);
        const char *sub = TextFormat("Gragas's final score: %d", world->gragas.score);
        Vector2 ssize = MeasureTextEx(f, sub, 22, 1);
        DrawTextEx(f, sub, (Vector2){WIDTH / 2 - ssize.x / 2, HEIGHT / 2}, 22, 1, RAYWHITE);
        const char *hint = "Press SPACE to play again";
        int hw = MeasureText(hint, 18);
        DrawText(hint, WIDTH / 2 - hw / 2, HEIGHT / 2 + 40, 18, LIGHTGRAY);
    }
}
