/*
** EPITECH PROJECT, 2022 (raylib port)
** gragas -- ported 1:1
*/
#include "my.h"

static void calculate_shadow(gragas_t *gragas)
{
    float radius = gragas->rect.y > 5 * 20 ? gragas->rect.y / 20 : 5;
    shadow_set_radius(&gragas->shadow, radius);
    frect_t shadow_rect = shadow_get_global_bounds(&gragas->shadow);
    v2f pos = { gragas->rect.x + gragas->rect.width / 2 -
    shadow_rect.width / 2, HEIGHT - G_FLOOR_HEIGHT - 40 };
    shadow_set_position(&gragas->shadow, pos);
}

static void stand_on_floor(gragas_t *gragas)
{
    if (gragas->rect.y > HEIGHT - gragas->rect.height - G_FLOOR_HEIGHT) {
        gragas->at_floor = true;
        if (gragas->jumping)
            return;
        if (gragas->spawning) {
            gragas->spawn_animation = 1;
            gragas->spawn_anim_clock = 0;
            gragas->has_spawn_anim_clock = true;
            gragas->spawning = false;
        }
        gragas->velocity.y = 0;
        sprite_set_position(&gragas->sp, (v2f){gragas->rect.x,
        HEIGHT - G_FLOOR_HEIGHT - gragas->rect.height});
    }
}

static void movement(gragas_t *gragas, float dt, float rawDt, barrel_t *barrel)
{
    if (!gragas->spawning && !gragas->jumping)
        goto_barrel(gragas, barrel, rawDt);

    gragas->velocity = int_multiply_v2f(gragas->velocity, GRAGAS_FRICTION);

    v2f movement = int_multiply_v2f(gragas->acceleration, dt);
    gragas->velocity = add_two_v2f(gragas->velocity, movement);
    if (gragas->sp.position.x > WIDTH)
        gragas->velocity.x = -1;

    stand_on_floor(gragas);

    sprite_move(&gragas->sp, int_multiply_v2f(gragas->velocity, dt));
}

static void jumping_gragas(gragas_t *gragas)
{
    if (gragas->jumping && gragas->at_floor) {
            gragas->jumping = false;
            gragas->rect_anim = (int_rect){0, 0, SQUATTING_GRAGAS_WIDTH,
                SQUATTING_GRAGAS_HEIGHT};
    }
}

void animate_gragas(gragas_t *gragas, float dt, float rawDt, barrel_t *barrel)
{
    if (gragas == NULL)
        return;
    gragas->rect = sprite_get_global_bounds(&gragas->sp);
    sprite_set_texture_rect(&gragas->sp, gragas->rect_anim);
    jumping_gragas(gragas);
    calculate_shadow(gragas);
    if (gragas->spawn_animation)
        animate_gragas_spawn(gragas, rawDt);
    else
        movement(gragas, dt, rawDt, barrel);
}
