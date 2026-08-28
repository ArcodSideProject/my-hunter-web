#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static float Vec2Dist(Vector2 a, Vector2 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

static Vector2 Vec2Norm(Vector2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.0001f) return (Vector2){0, 0};
    return (Vector2){v.x / len, v.y / len};
}

static void SpawnParticleBurst(GameWorld *world, Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; i++) {
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (!world->particles[p].active) {
                float angle = ((float)GetRandomValue(0, 360)) * DEG2RAD;
                float speed = (float)GetRandomValue(60, 220);
                world->particles[p] = (Particle){
                    .active = true,
                    .pos = pos,
                    .vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed},
                    .life = 0.5f,
                    .maxLife = 0.5f,
                    .color = color
                };
                break;
            }
        }
    }
}

static void ResetPlayer(Player *p) {
    p->pos = (Vector2){WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f};
    p->radius = 16.0f;
    p->hp = PLAYER_BASE_HP;
    p->maxHp = PLAYER_BASE_HP;
    p->speed = PLAYER_BASE_SPEED;
    p->fireCooldown = 0.0f;
    p->fireRate = 0.35f;
    p->level = 1;
    p->xp = 0;
    p->xpToNext = 10;
    p->bulletDamage = 10;
    p->invulnTimer = 0.0f;
}

void InitGameWorld(GameWorld *world) {
    memset(world, 0, sizeof(GameWorld));
    ResetPlayer(&world->player);
    world->state = STATE_MENU;
    world->wave = 0;
    world->waveTimer = 0.0f;
    world->spawnTimer = 0.0f;
    world->score = 0;
    world->gameTime = 0.0f;
    world->camera.zoom = 1.0f;
    world->camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    world->camera.target = world->player.pos;
    world->camera.rotation = 0.0f;
}

static void StartWave(GameWorld *world) {
    world->wave++;
    world->enemiesRemainingInWave = 5 + world->wave * 3;
    world->enemiesAliveCount = 0;
    world->spawnTimer = 0.0f;
}

static void SpawnEnemy(GameWorld *world) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!world->enemies[i].active) {
            float angle = ((float)GetRandomValue(0, 360)) * DEG2RAD;
            float dist = 500.0f + (float)GetRandomValue(0, 200);
            Vector2 pos = {
                world->player.pos.x + cosf(angle) * dist,
                world->player.pos.y + sinf(angle) * dist
            };

            EnemyType type = ENEMY_GRUNT;
            int roll = GetRandomValue(0, 100);
            bool isBossWave = (world->wave % 5 == 0) &&
                               (world->enemiesRemainingInWave == 1);
            if (isBossWave) {
                type = ENEMY_BOSS;
            } else if (roll < 15 && world->wave > 3) {
                type = ENEMY_TANK;
            } else if (roll < 40 && world->wave > 1) {
                type = ENEMY_RUNNER;
            }

            Enemy e = {0};
            e.active = true;
            e.pos = pos;
            e.type = type;
            switch (type) {
                case ENEMY_RUNNER:
                    e.radius = 10.0f; e.maxHp = 15 + world->wave * 2;
                    e.speed = 160.0f; break;
                case ENEMY_TANK:
                    e.radius = 22.0f; e.maxHp = 60 + world->wave * 5;
                    e.speed = 60.0f; break;
                case ENEMY_BOSS:
                    e.radius = 40.0f; e.maxHp = 300 + world->wave * 30;
                    e.speed = 70.0f; break;
                default:
                    e.radius = 14.0f; e.maxHp = 25 + world->wave * 3;
                    e.speed = 95.0f; break;
            }
            e.hp = e.maxHp;
            world->enemies[i] = e;
            world->enemiesAliveCount++;
            world->enemiesRemainingInWave--;
            return;
        }
    }
}

static void FireBullet(GameWorld *world, Vector2 dir) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!world->bullets[i].active) {
            world->bullets[i] = (Bullet){
                .active = true,
                .pos = world->player.pos,
                .vel = (Vector2){dir.x * BULLET_SPEED, dir.y * BULLET_SPEED},
                .radius = 5.0f,
                .damage = world->player.bulletDamage,
                .life = 1.5f
            };
            return;
        }
    }
}

static Enemy *NearestEnemy(GameWorld *world, Vector2 from) {
    Enemy *nearest = NULL;
    float bestDist = 1e9f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!world->enemies[i].active) continue;
        float d = Vec2Dist(from, world->enemies[i].pos);
        if (d < bestDist) {
            bestDist = d;
            nearest = &world->enemies[i];
        }
    }
    return nearest;
}

static void UpdatePlaying(GameWorld *world, float dt) {
    Player *p = &world->player;
    world->gameTime += dt;

    // movement
    Vector2 move = {0, 0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1;
    move = Vec2Norm(move);
    p->pos.x += move.x * p->speed * dt;
    p->pos.y += move.y * p->speed * dt;

    if (p->pos.x < 0) p->pos.x = 0;
    if (p->pos.y < 0) p->pos.y = 0;
    if (p->pos.x > WORLD_WIDTH) p->pos.x = WORLD_WIDTH;
    if (p->pos.y > WORLD_HEIGHT) p->pos.y = WORLD_HEIGHT;

    world->camera.target = p->pos;

    if (p->invulnTimer > 0) p->invulnTimer -= dt;

    // auto-fire at nearest enemy
    p->fireCooldown -= dt;
    if (p->fireCooldown <= 0) {
        Enemy *target = NearestEnemy(world, p->pos);
        if (target) {
            Vector2 dir = Vec2Norm((Vector2){
                target->pos.x - p->pos.x,
                target->pos.y - p->pos.y
            });
            FireBullet(world, dir);
            p->fireCooldown = p->fireRate;
        }
    }

    // spawn logic
    world->spawnTimer -= dt;
    if (world->spawnTimer <= 0 && world->enemiesRemainingInWave > 0) {
        SpawnEnemy(world);
        world->spawnTimer = 0.6f;
    }
    if (world->enemiesRemainingInWave <= 0 && world->enemiesAliveCount <= 0) {
        StartWave(world);
    }

    // bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &world->bullets[i];
        if (!b->active) continue;
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;
        b->life -= dt;
        if (b->life <= 0) { b->active = false; continue; }

        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &world->enemies[j];
            if (!e->active) continue;
            if (Vec2Dist(b->pos, e->pos) < b->radius + e->radius) {
                e->hp -= b->damage;
                e->hitFlash = 0.1f;
                b->active = false;
                SpawnParticleBurst(world, b->pos, YELLOW, 4);
                if (e->hp <= 0) {
                    e->active = false;
                    world->enemiesAliveCount--;
                    world->score += (e->type == ENEMY_BOSS) ? 500 :
                                     (e->type == ENEMY_TANK) ? 50 : 10;
                    p->xp += (e->type == ENEMY_BOSS) ? 50 :
                              (e->type == ENEMY_TANK) ? 8 : 3;
                    SpawnParticleBurst(world, e->pos, RED, 12);
                    if (p->xp >= p->xpToNext) {
                        p->xp -= p->xpToNext;
                        p->xpToNext = (int)(p->xpToNext * 1.35f) + 5;
                        p->level++;
                        world->state = STATE_LEVEL_UP;
                    }
                }
                break;
            }
        }
    }

    // enemies chase player
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &world->enemies[i];
        if (!e->active) continue;
        if (e->hitFlash > 0) e->hitFlash -= dt;

        Vector2 dir = Vec2Norm((Vector2){p->pos.x - e->pos.x, p->pos.y - e->pos.y});
        e->pos.x += dir.x * e->speed * dt;
        e->pos.y += dir.y * e->speed * dt;

        if (Vec2Dist(e->pos, p->pos) < e->radius + p->radius) {
            if (p->invulnTimer <= 0) {
                int dmg = (e->type == ENEMY_BOSS) ? 25 :
                          (e->type == ENEMY_TANK) ? 15 : 8;
                p->hp -= dmg;
                p->invulnTimer = 0.6f;
                SpawnParticleBurst(world, p->pos, ORANGE, 8);
                if (p->hp <= 0) {
                    world->state = STATE_GAME_OVER;
                }
            }
        }
    }

    // particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &world->particles[i];
        if (!pt->active) continue;
        pt->pos.x += pt->vel.x * dt;
        pt->pos.y += pt->vel.y * dt;
        pt->vel.x *= 0.92f;
        pt->vel.y *= 0.92f;
        pt->life -= dt;
        if (pt->life <= 0) pt->active = false;
    }
}

void UpdateGameWorld(GameWorld *world, float dt) {
    switch (world->state) {
        case STATE_MENU:
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                InitGameWorld(world);
                world->state = STATE_PLAYING;
                StartWave(world);
            }
            break;
        case STATE_PLAYING:
            UpdatePlaying(world, dt);
            break;
        case STATE_LEVEL_UP:
            if (IsKeyPressed(KEY_ONE)) {
                world->player.bulletDamage += 5;
                world->state = STATE_PLAYING;
            } else if (IsKeyPressed(KEY_TWO)) {
                world->player.fireRate *= 0.85f;
                world->state = STATE_PLAYING;
            } else if (IsKeyPressed(KEY_THREE)) {
                world->player.speed += 25.0f;
                world->player.maxHp += 15;
                world->player.hp += 15;
                world->state = STATE_PLAYING;
            }
            break;
        case STATE_GAME_OVER:
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                InitGameWorld(world);
                world->state = STATE_MENU;
            }
            break;
    }
}

static Color EnemyColor(EnemyType type) {
    switch (type) {
        case ENEMY_RUNNER: return SKYBLUE;
        case ENEMY_TANK: return PURPLE;
        case ENEMY_BOSS: return MAROON;
        default: return LIME;
    }
}

static void DrawHUD(GameWorld *world) {
    Player *p = &world->player;
    DrawRectangle(10, 10, 220, 20, DARKGRAY);
    DrawRectangle(10, 10, (int)(220.0f * p->hp / p->maxHp), 20, RED);
    DrawRectangleLines(10, 10, 220, 20, WHITE);
    DrawText(TextFormat("HP %d/%d", p->hp, p->maxHp), 15, 12, 16, WHITE);

    DrawRectangle(10, 35, 220, 12, DARKGRAY);
    DrawRectangle(10, 35, (int)(220.0f * p->xp / p->xpToNext), 12, SKYBLUE);
    DrawRectangleLines(10, 35, 220, 12, WHITE);

    DrawText(TextFormat("Wave %d   Score %d   Lv.%d", world->wave, world->score, p->level),
              10, 55, 18, WHITE);
    DrawText(TextFormat("Enemies left: %d", world->enemiesRemainingInWave + world->enemiesAliveCount),
              10, 78, 16, LIGHTGRAY);
}

void DrawGameWorld(GameWorld *world) {
    ClearBackground((Color){20, 20, 28, 255});

    if (world->state == STATE_MENU) {
        const char *title = "MY HUNTER (web)";
        int tw = MeasureText(title, 48);
        DrawText(title, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 80, 48, RAYWHITE);
        const char *sub = "Press SPACE to start   |   WASD/Arrows to move, auto-fire";
        int sw = MeasureText(sub, 20);
        DrawText(sub, GetScreenWidth() / 2 - sw / 2, GetScreenHeight() / 2, 20, LIGHTGRAY);
        return;
    }

    BeginMode2D(world->camera);

    DrawRectangleLines(0, 0, WORLD_WIDTH, WORLD_HEIGHT, DARKGRAY);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &world->particles[i];
        if (!pt->active) continue;
        float alpha = pt->life / pt->maxLife;
        Color c = pt->color;
        c.a = (unsigned char)(alpha * 255);
        DrawCircleV(pt->pos, 3.0f * alpha, c);
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &world->enemies[i];
        if (!e->active) continue;
        Color c = e->hitFlash > 0 ? WHITE : EnemyColor(e->type);
        DrawCircleV(e->pos, e->radius, c);
        float hpFrac = (float)e->hp / e->maxHp;
        DrawRectangle((int)(e->pos.x - e->radius), (int)(e->pos.y - e->radius - 8),
                      (int)(e->radius * 2), 4, DARKGRAY);
        DrawRectangle((int)(e->pos.x - e->radius), (int)(e->pos.y - e->radius - 8),
                      (int)(e->radius * 2 * hpFrac), 4, RED);
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &world->bullets[i];
        if (!b->active) continue;
        DrawCircleV(b->pos, b->radius, YELLOW);
    }

    Player *p = &world->player;
    Color pc = (p->invulnTimer > 0 && ((int)(p->invulnTimer * 20) % 2 == 0)) ? WHITE : GREEN;
    DrawCircleV(p->pos, p->radius, pc);
    DrawCircleLines((int)p->pos.x, (int)p->pos.y, p->radius, RAYWHITE);

    EndMode2D();

    DrawHUD(world);

    if (world->state == STATE_LEVEL_UP) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 180});
        const char *title = "LEVEL UP! Choose an upgrade:";
        int tw = MeasureText(title, 32);
        DrawText(title, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 100, 32, GOLD);
        DrawText("[1] +5 Bullet Damage", GetScreenWidth() / 2 - 140, GetScreenHeight() / 2 - 30, 22, RAYWHITE);
        DrawText("[2] +15% Fire Rate", GetScreenWidth() / 2 - 140, GetScreenHeight() / 2 + 5, 22, RAYWHITE);
        DrawText("[3] +Speed & +15 Max HP", GetScreenWidth() / 2 - 140, GetScreenHeight() / 2 + 40, 22, RAYWHITE);
    }

    if (world->state == STATE_GAME_OVER) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 180});
        const char *title = "GAME OVER";
        int tw = MeasureText(title, 48);
        DrawText(title, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 60, 48, RED);
        const char *score = TextFormat("Final Score: %d   Wave Reached: %d", world->score, world->wave);
        int sw = MeasureText(score, 22);
        DrawText(score, GetScreenWidth() / 2 - sw / 2, GetScreenHeight() / 2, 22, RAYWHITE);
        const char *sub = "Press SPACE to return to menu";
        int subw = MeasureText(sub, 18);
        DrawText(sub, GetScreenWidth() / 2 - subw / 2, GetScreenHeight() / 2 + 40, 18, LIGHTGRAY);
    }
}
