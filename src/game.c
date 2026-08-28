#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// barrel.png is 22x28 (taller than wide), scaled 3x per the original's
// `sfVector2f barrel_scale = {3, 3}` -> 66x84 on screen. Using a square
// 60x60 box here (the earlier version of this file) stretched the sprite
// into a square, which is why barrels looked square instead of the
// original's slightly tall/rectangular shape.
#define BARREL_W 66
#define BARREL_H 84
#define GRAGAS_W 100  // 2x scale of ~50px squat sprite
#define GRAGAS_H 88

// ---- vector helpers (mirrors v2f_operations.c) ----
static Vector2 V2Mul(Vector2 v, float s) { return (Vector2){v.x * s, v.y * s}; }
static Vector2 V2Add(Vector2 a, Vector2 b) { return (Vector2){a.x + b.x, a.y + b.y}; }
static float AbsF(float x) { return x < 0 ? -x : x; }

// ---- pixel-space (Y-down, top-left origin) <-> Box2D meter-space (Y-up)
// conversion helpers. `pixelTopLeft` is the sprite's top-left corner in
// game/screen coordinates; Box2D bodies are positioned by their CENTER.
static b2Vec2 PixelTopLeftToB2Center(Vector2 topLeft, float w, float h) {
    float cx = topLeft.x + w / 2.0f;
    float cy = topLeft.y + h / 2.0f;
    return (b2Vec2){ cx / PIXELS_PER_METER, (HEIGHT - cy) / PIXELS_PER_METER };
}
static Vector2 B2CenterToPixelTopLeft(b2Vec2 center, float w, float h) {
    float px = center.x * PIXELS_PER_METER;
    float py = HEIGHT - center.y * PIXELS_PER_METER;
    return (Vector2){ px - w / 2.0f, py - h / 2.0f };
}

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
    // Destroy any existing physics world/bodies before the memset wipes
    // their handles out from under us -- b2WorldId 0 (default-zeroed) is
    // not a valid handle, so without this we'd leak the old world on
    // every restart (STATE_GAME_OVER -> SPACE/ENTER).
    if (b2World_IsValid(world->physicsWorld)) {
        DestroyPhysicsWorld(world);
    }
    memset(world, 0, sizeof(GameWorld));
    world->assets = savedAssets;
    world->state = STATE_MENU;
    world->round = 0;
    world->cloudsSpeed = 20.0f;
    world->treeTransparency = 255.0f;
    InitPhysicsWorld(world);
}

void InitPhysicsWorld(GameWorld *world) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){ 0.0f, -20.0f }; // m/s^2, tuned to feel close to the original's snappy arcade fall speed
    world->physicsWorld = b2CreateWorld(&worldDef);

    // Static floor: a thin box spanning the play area width, top edge at
    // the same floor line barrels used to clamp to by hand
    // (HEIGHT - FLOOR_HEIGHT), so Gragas/round logic (which still checks
    // pixel Y) doesn't need to change.
    float floorTopY = HEIGHT - FLOOR_HEIGHT;
    b2BodyDef floorDef = b2DefaultBodyDef();
    floorDef.position = PixelTopLeftToB2Center((Vector2){0, floorTopY}, WIDTH, 40.0f);
    world->floorBody = b2CreateBody(world->physicsWorld, &floorDef);
    b2Polygon floorBox = b2MakeBox((WIDTH / 2.0f) / PIXELS_PER_METER, 20.0f / PIXELS_PER_METER);
    b2ShapeDef floorShapeDef = b2DefaultShapeDef();
    floorShapeDef.material.friction = 0.5f;
    b2CreatePolygonShape(world->floorBody, &floorShapeDef, &floorBox);

    // Static side walls, matching bounce_on_border's `pos.x < 0` /
    // `pos.x > WIDTH - width` clamps -- tall thin boxes just outside the
    // play area so barrels bounce off the screen edges instead of
    // flying off it.
    b2BodyDef leftWallDef = b2DefaultBodyDef();
    leftWallDef.position = PixelTopLeftToB2Center((Vector2){-40, -2000}, 40.0f, 4000.0f);
    world->leftWallBody = b2CreateBody(world->physicsWorld, &leftWallDef);
    b2Polygon leftWallBox = b2MakeBox(20.0f / PIXELS_PER_METER, 2000.0f / PIXELS_PER_METER);
    b2ShapeDef wallShapeDef = b2DefaultShapeDef();
    wallShapeDef.material.friction = 0.1f;
    b2CreatePolygonShape(world->leftWallBody, &wallShapeDef, &leftWallBox);

    b2BodyDef rightWallDef = b2DefaultBodyDef();
    rightWallDef.position = PixelTopLeftToB2Center((Vector2){WIDTH, -2000}, 40.0f, 4000.0f);
    world->rightWallBody = b2CreateBody(world->physicsWorld, &rightWallDef);
    b2Polygon rightWallBox = b2MakeBox(20.0f / PIXELS_PER_METER, 2000.0f / PIXELS_PER_METER);
    b2CreatePolygonShape(world->rightWallBody, &wallShapeDef, &rightWallBox);
}

void DestroyPhysicsWorld(GameWorld *world) {
    if (b2World_IsValid(world->physicsWorld)) {
        b2DestroyWorld(world->physicsWorld);
    }
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
    // if this slot held a previous barrel whose body wasn't cleaned up
    // (shouldn't normally happen -- AnimateExplosion destroys it on
    // finishing -- but guard against a leak regardless)
    if (b2Body_IsValid(b->bodyId)) {
        b2DestroyBody(b->bodyId);
    }
    memset(b, 0, sizeof(Barrel));
    b->active = true;
    b->spawning = true;
    b->pos = (Vector2){(float)GetRandomValue(0, WIDTH), HEIGHT + 50.0f};
    b->maxHealth = maxHealth;
    b->health = maxHealth;
    b->dead = false;
    world->barrelCount++;
    world->barrelsSpawned++;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = PixelTopLeftToB2Center(b->pos, BARREL_W, BARREL_H);
    // initial velocity: original used {rand()%100-50, -110} in its own
    // tick-scaled units; Box2D steps with real (unscaled) dt, so this is
    // retuned in real m/s to produce a comparable snappy arcade launch,
    // not a literal unit conversion of the original constant.
    float vx = (float)(GetRandomValue(0, 100) - 50) * 0.3f;
    float vy = 9.0f; // upward in Box2D's Y-up world (m/s)
    bodyDef.linearVelocity = (b2Vec2){ vx, vy };
    bodyDef.angularDamping = 0.0f;
    b->bodyId = b2CreateBody(world->physicsWorld, &bodyDef);

    // Barrel collision shape sized to match its on-screen box, minus a
    // small margin so visually-touching barrels don't feel like they're
    // colliding early.
    b2Polygon barrelBox = b2MakeBox((BARREL_W / 2.0f - 2.0f) / PIXELS_PER_METER,
                                     (BARREL_H / 2.0f - 2.0f) / PIXELS_PER_METER);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.4f;   // matches FLOOR_FRICTION's spirit
    shapeDef.material.restitution = 0.15f; // slight bounce, not too bouncy
    b2CreatePolygonShape(b->bodyId, &shapeDef, &barrelBox);
}

// ---- barrels: Box2D now owns gravity, floor contact, wall bounce, and
// barrel-vs-barrel collision (the original's hand-rolled bounce.c/
// barrels.c logic had none of that -- overlapping barrels would visually
// sink into each other instead of settling side by side). This section
// just syncs each Barrel's cached pixel-space pos/rotation/atFloor from
// its Box2D body once per frame, after b2World_Step has run.
#define AT_FLOOR_SPEED_THRESHOLD 0.15f // m/s, "basically not moving" cutoff

static void SyncBarrelFromPhysics(Barrel *b) {
    if (!b2Body_IsValid(b->bodyId)) return;

    b2Vec2 center = b2Body_GetPosition(b->bodyId);
    b->pos = B2CenterToPixelTopLeft(center, BARREL_W, BARREL_H);

    b2Rot rot = b2Body_GetRotation(b->bodyId);
    b->rotation = -b2Rot_GetAngle(rot) * (180.0f / (float)M_PI); // negate: Box2D CCW+ vs screen-space CW+

    b2Vec2 vel = b2Body_GetLinearVelocity(b->bodyId);
    float speed = sqrtf(vel.x * vel.x + vel.y * vel.y);
    // "at floor" = resting near the ground line and not actively moving --
    // used by Gragas's AI to pick a target, matching the original's
    // at_floor flag semantics (only chase barrels that have landed).
    float floorPos = HEIGHT - BARREL_H - FLOOR_HEIGHT;
    b->atFloor = (!b->spawning) && (b->pos.y >= floorPos - 4.0f) && (speed < AT_FLOOR_SPEED_THRESHOLD);
}

static void AnimateBarrel(Barrel *b, float dt) {
    b->spawnTimer += dt; // real time, matches sfClock_getElapsedTime > 500000us
    if (b->spawnTimer > 0.5f) b->spawning = false;
    SyncBarrelFromPhysics(b);
}

#define EXPLOSION_FRAME_TIME 0.035f
#define EXPLOSION_FRAMES 6  // real frame count in explosion.png (315px / (50+3)px stride)

static void AnimateExplosion(Barrel *b, float dt) {
    b->explosionTimer += dt;
    if (b->explosionTimer > EXPLOSION_FRAME_TIME) {
        b->explosionFrame++;
        b->explosionTimer = 0;
        if (b->explosionFrame >= EXPLOSION_FRAMES) {
            // done exploding: destroy the Box2D body (it was already
            // removed from simulation semantics the moment it "died",
            // but the body itself needs explicit cleanup) and free the slot
            if (b2Body_IsValid(b->bodyId)) {
                b2DestroyBody(b->bodyId);
                b->bodyId = b2_nullBodyId;
            }
            b->active = false;
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
            if (b2Body_IsValid(target->bodyId)) b2Body_Disable(target->bodyId);
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
    if (b2Body_IsValid(b->bodyId)) {
        // Knock the barrel upward with a random horizontal kick, matching
        // the spirit of the original's `velocity = {-60y-ish kick, random
        // x}` -- applied as an impulse (mass-independent enough at
        // density=1) rather than a raw velocity set, since Box2D bodies
        // don't expose a bare "velocity" field to assign to directly the
        // way the hand-rolled physics did.
        float impulseX = 6.0f * ((GetRandomValue(0, 1) == 0) ? 1.0f : -1.0f);
        float impulseY = 4.0f;
        b2Body_ApplyLinearImpulseToCenter(b->bodyId, (b2Vec2){impulseX, impulseY}, true);
    }
    b->health--;
    if (b->health <= 0) {
        b->dead = true;
        b->explosionFrame = 0;
        b->explosionTimer = 0;
        if (b2Body_IsValid(b->bodyId)) b2Body_Disable(b->bodyId);
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
    // not a test-only shortcut. Extended so holding the key spawns faster
    // and faster the longer it's held: starts at one spawn every 0.35s,
    // ramps down to one every 0.02s (50/s) over 3 seconds of continuous
    // hold -- noticeably more aggressive acceleration curve + higher cap
    // per explicit request.
    if (world->state == STATE_PLAYING) {
        if (IsKeyPressed(KEY_ENTER)) {
            SpawnBarrel(world, 1);
            world->enterHoldTime = 0;
            world->enterSpawnAccum = 0;
        } else if (IsKeyDown(KEY_ENTER)) {
            world->enterHoldTime += dt;
            world->enterSpawnAccum += dt;
            float t = world->enterHoldTime / 3.0f;
            if (t > 1.0f) t = 1.0f;
            float interval = 0.35f - t * (0.35f - 0.02f);
            // fire multiple spawns in one frame if the accumulated time
            // covers more than one interval (keeps up at very high rates
            // even if frame time is coarse)
            while (world->enterSpawnAccum > interval) {
                SpawnBarrel(world, 1);
                world->enterSpawnAccum -= interval;
            }
        } else {
            world->enterHoldTime = 0;
            world->enterSpawnAccum = 0;
        }
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

    // Step the Box2D world with REAL dt (physics engines integrate in
    // real time; GAME_TICK scaling only applies to the original's own
    // hand-rolled formulas, which still apply to Gragas's custom AI
    // movement below via physicsDt).
    if (b2World_IsValid(world->physicsWorld)) {
        b2World_Step(world->physicsWorld, dt, 4);
    }
    UpdateBarrels(world, dt);
    AnimateGragas(world, dt, physicsDt);
}

// ---- drawing ----
// ---- background layers, split to match the original's actual draw order
// (draw.c): sky/clouds/mountain/far_woods first, THEN spawning barrels,
// THEN tiles (ground), THEN settled barrels, THEN Gragas, THEN tree,
// THEN front_grass (in front of everything), THEN the mouse sight.
// A single flat "draw all background, then all barrels/gragas" pass (the
// earlier version of this file) put front_grass behind the barrels and
// merged the two barrel layers, which is why grass appeared behind
// barrels instead of in front.
static void DrawSkyLayers(GameWorld *world) {
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
        return;
    }

    Rectangle dst = {0, 0, WIDTH, HEIGHT};
    DrawTexturePro(a->sky, (Rectangle){0, 0, (float)a->sky.width, (float)a->sky.height},
                    dst, (Vector2){0, 0}, 0, WHITE);

    float cx = fmodf(world->cloudsX, WIDTH);
    Rectangle cloudsSrc = {0, 0, (float)a->clouds.width, (float)a->clouds.height};
    DrawTexturePro(a->clouds, cloudsSrc, (Rectangle){cx, 0, WIDTH, HEIGHT}, (Vector2){0, 0}, 0, WHITE);
    DrawTexturePro(a->clouds, cloudsSrc, (Rectangle){cx - WIDTH, 0, WIDTH, HEIGHT}, (Vector2){0, 0}, 0, WHITE);

    DrawTexturePro(a->mountain, (Rectangle){0, 0, (float)a->mountain.width, (float)a->mountain.height},
                    dst, (Vector2){0, 0}, 0, WHITE);
    DrawTexturePro(a->farWoods, (Rectangle){0, 0, (float)a->farWoods.width, (float)a->farWoods.height},
                    dst, (Vector2){0, 0}, 0, WHITE);
}

static void DrawGroundLayer(GameWorld *world) {
    Assets *a = &world->assets;
    if (!a->loaded) {
        DrawRectangle(0, HEIGHT - FLOOR_HEIGHT, WIDTH, FLOOR_HEIGHT, (Color){90, 140, 60, 255});
        return;
    }
    Rectangle dst = {0, 0, WIDTH, HEIGHT};
    DrawTexturePro(a->tiles, (Rectangle){0, 0, (float)a->tiles.width, (float)a->tiles.height},
                    dst, (Vector2){0, 0}, 0, WHITE);
}

static void DrawTreeAndGrass(GameWorld *world) {
    Assets *a = &world->assets;
    if (!a->loaded) return;
    float scaleX = WIDTH / 384.0f, scaleY = HEIGHT / 224.0f;
    DrawTextureEx(a->tree, (Vector2){TREE_OFFSET_X * scaleX, TREE_OFFSET_Y * scaleY}, 0, scaleX,
                  (Color){255, 255, 255, (unsigned char)world->treeTransparency});
    Rectangle dst = {0, 0, WIDTH, HEIGHT};
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

    // shadow, matching calculate_barrel_shadow: fixed at floor level
    // (HEIGHT - FLOOR_HEIGHT - 45), NOT following the barrel's current Y --
    // the original's shadow only ever sits on the ground, shrinking/growing
    // by how high the barrel currently is (rect.top/20), which reads as a
    // classic "shadow on the floor below a jumping object" effect.
    float shadowY = HEIGHT - FLOOR_HEIGHT - 45.0f;
    float shadowRadius = (r.y > 5 * 20) ? r.y / 20.0f : 5.0f;
    DrawEllipse((int)center.x, (int)shadowY, (int)shadowRadius, (int)(shadowRadius * 0.4f),
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

    // shadow, matching calculate_shadow: fixed at HEIGHT - G_FLOOR_HEIGHT - 40,
    // radius grows with height off the ground (rect.top/20), same formula
    // as the barrel shadow.
    float gShadowY = HEIGHT - G_FLOOR_HEIGHT - 40.0f;
    float gShadowRadius = (r.y > 5 * 20) ? r.y / 20.0f : 5.0f;
    DrawEllipse((int)(r.x + r.width / 2), (int)gShadowY,
                (int)gShadowRadius, (int)(gShadowRadius * 0.4f), (Color){0, 0, 0, 60});
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

    // Render at the frame's NATIVE size * the original's fixed 2x scale
    // (`create_sprite(gragas_texture, (v2f){2, 2})`), NOT stretched into
    // a single fixed box. Squatting (50x44) and standing (66x62) frames
    // are genuinely different sizes in the original -- forcing them both
    // into one fixed destination rect (GragasRect(), used for physics)
    // squashed/stretched the sprite and visibly shifted its apparent
    // vertical position every time the animation state changed (spawn
    // <-> walk, jump <-> stand), which is what read as "weird up and
    // down jumps". Anchor to the bottom-center of the physics rect so
    // the feet stay planted on the ground regardless of frame height.
    float renderW = src.width * 2.0f;
    float renderH = src.height * 2.0f;
    Rectangle dst = {
        r.x + r.width / 2 - renderW / 2,
        r.y + r.height - renderH,
        renderW, renderH
    };
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
    DrawSkyLayers(world);
    Assets *a = &world->assets;
    Font f = a->loaded ? a->font : GetFontDefault();

    if (world->state == STATE_MENU) {
        DrawGroundLayer(world);
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
        DrawTreeAndGrass(world);
        return;
    }

    // Real draw order, matching draw_on_screen() in draw.c exactly:
    // sky layers (done above) -> spawning barrels -> ground/tiles ->
    // settled barrels -> Gragas -> tree -> front_grass (in front of
    // everything drawn so far) -> HUD text on top of that.
    for (int i = 0; i < MAX_BARRELS; i++) {
        Barrel *b = &world->barrels[i];
        if (b->active && b->spawning) DrawBarrel(world, b);
    }
    DrawGroundLayer(world);
    for (int i = 0; i < MAX_BARRELS; i++) {
        Barrel *b = &world->barrels[i];
        if (b->active && !b->spawning) DrawBarrel(world, b);
    }
    DrawGragas(world);
    DrawTreeAndGrass(world);
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
