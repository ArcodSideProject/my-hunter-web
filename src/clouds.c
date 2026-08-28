/*
** EPITECH PROJECT, 2022 (raylib port)
** clouds -- ported 1:1
*/
#include "my.h"

void move_clouds(rsprite_t *clouds, float *clouds_speed, int score)
{
    if (*clouds_speed > score) {
        (*clouds_speed) -= 0.7f;
    } else {
        *clouds_speed = (float)score;
    }
    sprite_move(clouds, (v2f){(*clouds_speed) + 1, 0});
    if (sprite_get_global_bounds(clouds).x > WIDTH)
        sprite_set_position(clouds, (v2f){-WIDTH, 0});
}
