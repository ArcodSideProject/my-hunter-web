/*
** EPITECH PROJECT, 2022 (raylib port)
** create_start_button -- ported 1:1
*/
#include "my.h"

rsprite_t create_start_button(Texture2D start_button_texture)
{
    rsprite_t start_button = create_sprite(start_button_texture, (v2f){2, 2});
    frect_t rect = sprite_get_global_bounds(&start_button);

    sprite_set_position(&start_button, (v2f){WIDTH / 2 - rect.width / 2,
    HEIGHT / 2 - rect.height / 2});
    return start_button;
}
