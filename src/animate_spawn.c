/*
** EPITECH PROJECT, 2022 (raylib port)
** animate_spawn -- ported 1:1
*/
#include "my.h"

void animate_gragas_spawn(gragas_t *gragas, float rawDt)
{
    gragas->spawn_anim_clock += rawDt;
    float sec = gragas->spawn_anim_clock;
    if (gragas->spawn_animation == 3 && sec > 0.50f) {
        gragas->rect_anim = (int_rect){0, 0, SQUATTING_GRAGAS_WIDTH,
        SQUATTING_GRAGAS_HEIGHT};
        gragas->has_spawn_anim_clock = false;
        gragas->spawn_animation = 0;
    } if (gragas->spawn_animation == 2 && sec > 0.30f) {
        gragas->rect_anim.x += STANDING_GRAGAS_WIDTH + 2;
        gragas->spawn_anim_clock = 0;
        gragas->spawn_animation++;
    } if (gragas->spawn_animation == 1 && sec > 0.07f) {
        gragas->rect_anim.x += STANDING_GRAGAS_WIDTH + 2;
        gragas->spawn_anim_clock = 0;
        gragas->spawn_animation++;
    }
}
