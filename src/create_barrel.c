/*
** EPITECH PROJECT, 2022 (raylib port)
** create_barrel -- ported 1:1
*/
#include "my.h"

static void spawn_pos_speed_color(barrel_t *barrel)
{
    barrel->spawning = true;
    barrel->clock = 0;
    sprite_set_position(&barrel->sp, (v2f){(float)(rand() % WIDTH), HEIGHT + 50});
    barrel->acceleration = (v2f){0, GRAVITY};
    barrel->velocity = (v2f){(float)(rand() % 100 - 50), -110};
    sprite_set_color(&barrel->sp, (Color){120, 120, 120, 220});
}

static void adapt_origin_to_rotate(barrel_t *barrel)
{
    barrel->rect = sprite_get_global_bounds(&barrel->sp);
    v2f origin = { barrel->rect.width / (2 * barrel->scale.x),
    barrel->rect.height / (2 * barrel->scale.y) };
    sprite_set_origin(&barrel->sp, origin);
}

void create_barrel(barrel_t *barrel, int max_health, Texture2D barrel_texture,
Texture2D explosion_texture)
{
    v2f barrel_scale = { 3, 3 };
    barrel->sp = create_sprite(barrel_texture, barrel_scale);
    barrel->scale = barrel_scale;

    spawn_pos_speed_color(barrel);

    adapt_origin_to_rotate(barrel);
    barrel->shadow = create_shadow((v2f){1.7f, 1});

    barrel->max_health = max_health;
    barrel->health = barrel->max_health;
    barrel->dead = false;
    barrel->explosion_state = 0;
    barrel->explosion_texture = explosion_texture;
    barrel->at_floor = false;
    barrel->rect_anim = (int_rect){0, 0, EXPLOSION_WIDTH, EXPLOSION_HEIGHT};
    barrel->next_barrel = NULL;
}
