/*
** EPITECH PROJECT, 2022 (raylib port)
** create_gragas -- ported 1:1
*/
#include "my.h"

void create_gragas(gragas_t *gragas, Texture2D gragas_texture)
{
    gragas->sp = create_sprite(gragas_texture, (v2f){2, 2});

    gragas->spawning = true;
    gragas->clock = 0;
    gragas->jumping = false;
    gragas->at_floor = false;
    gragas->acceleration = (v2f){0, GRAVITY * 2};
    gragas->velocity = (v2f){0, 250};
    gragas->rect_anim = (int_rect){0, STANDING_GRAGAS_HEIGHT_OFFSET,
    STANDING_GRAGAS_WIDTH, STANDING_GRAGAS_HEIGHT};
    sprite_set_texture_rect(&gragas->sp, gragas->rect_anim);
    gragas->rect = sprite_get_global_bounds(&gragas->sp);
    sprite_set_position(&gragas->sp, (v2f){WIDTH / 3, -100});
    gragas->spawn_animation = 0;
    gragas->has_spawn_anim_clock = false;
    gragas->gscore = malloc(sizeof(textint_t));
    char *gscore_string = "Gragas's score: ";
    create_text(gragas->gscore, gscore_string, (v2f){20, 40});
    gragas->shadow = create_shadow((v2f){2.1f, 1.2f});
}
