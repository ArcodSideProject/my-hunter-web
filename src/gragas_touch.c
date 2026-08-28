/*
** EPITECH PROJECT, 2023 (raylib port)
** gragas_touch -- ported 1:1
*/
#include "my.h"

void gragas_touch(gragas_t *gragas, Vector2 mpos)
{
    if (gragas->spawning || gragas->spawn_animation)
        return;
    if (frect_contains(&gragas->rect, (int)mpos.x, (int)mpos.y)) {
        gragas->velocity = (v2f){20, -80};
        if (!gragas->jumping) {
            gragas->acceleration.x = 0;
            gragas->jumping = true;
            gragas->at_floor = false;
            gragas->rect_anim = (int_rect){210, 0, SQUATTING_GRAGAS_WIDTH,
                SQUATTING_GRAGAS_HEIGHT};
            sprite_move(&gragas->sp, (v2f){0, -40});
        }
    }
}
