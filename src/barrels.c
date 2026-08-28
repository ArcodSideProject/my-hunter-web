/*
** EPITECH PROJECT, 2022 (raylib port)
** barrel
** File description:
** barrel -- ported 1:1 from the original; sfClock_getElapsedTime is
** replaced by a plain float seconds counter (barrel->clock), advanced
** by the real (unscaled) per-frame delta each call, matching what the
** SFML clock measured (real elapsed wall-clock time).
*/
#include "my.h"

static void animate_flying_barrel(barrel_t *barrel, float dt, float rawDt)
{
    barrel->velocity = int_multiply_v2f(barrel->velocity, AIR_FRICTION);

    v2f movement = int_multiply_v2f(barrel->acceleration, dt);
    barrel->velocity = add_two_v2f(barrel->velocity, movement);

    bounce_on_border(barrel, rawDt);

    sprite_move(&barrel->sp, int_multiply_v2f(barrel->velocity, dt));
}

static void animate_barrel(barrel_t *barrel, float dt, float rawDt)
{
    barrel->clock += rawDt;
    if (barrel->clock > 0.5f) {
        sprite_set_color(&barrel->sp, WHITE);
        barrel->spawning = false;
    }
    barrel->rect = sprite_get_global_bounds(&barrel->sp);
    sprite_rotate(&barrel->sp, barrel->velocity.x / 2);
    animate_flying_barrel(barrel, dt, rawDt);
    calculate_barrel_shadow(barrel);
}

static void animate_explosion(barrel_t *barrel, float rawDt)
{
    float sec = barrel->clock;
    barrel->clock += rawDt;
    sprite_set_texture_rect(&barrel->sp, barrel->rect_anim);
    // sped up ~13% per explicit request (0.035s -> 0.0305s per frame)
    if (sec > 0.0305f) {
        barrel->rect_anim.x += EXPLOSION_WIDTH + 3;
        barrel->clock = 0;
        barrel->explosion_state += 1;
    }
}

void animate_barrels(int barrel_count, barrel_t *barrel, float dt, float rawDt)
{
    if (barrel_count <= 0 || barrel == NULL)
        return;
    do {
        if (barrel->dead) {
            animate_explosion(barrel, rawDt);
        } else {
            animate_barrel(barrel, dt, rawDt);
        } if (!barrel->spawning && !barrel->dead) {
            sprite_set_color(&barrel->sp, (Color){255,
            (unsigned char)(barrel->health * 255 / barrel->max_health),
            (unsigned char)(barrel->health * 255 / barrel->max_health), 255});
        }
        barrel = barrel->next_barrel;
    } while (barrel != NULL);
}
