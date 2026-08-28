/*
** EPITECH PROJECT, 2022 (raylib port)
** goto_barrel -- ported 1:1
*/
#include "my.h"

static float my_round(float number)
{
    int int_number = (int)number;
    if (int_number % 100 > 50)
        int_number += 100 - int_number % 100;
    else
        int_number -= int_number % 100;
    return (float)int_number;
}

static void walking_animation(gragas_t *gragas, float rawDt)
{
    gragas->clock += rawDt;
    float sec = gragas->clock;
    if (sec > 0.05f + (1.0f / (ABS(gragas->velocity.x)))) {
        gragas->rect_anim.x += SQUATTING_GRAGAS_WIDTH + 2;
        gragas->rect_anim.x = (int)gragas->rect_anim.x % ((SQUATTING_GRAGAS_WIDTH + 2) * 3);
        gragas->clock = 0;
    }
}

static void touching_barrel(gragas_t *gragas, barrel_t *barrel)
{
    if (my_round(gragas->rect.x) == my_round(barrel->rect.x)) {
        if (!barrel->dead) {
            kill_barrel(barrel);
            gragas->gscore->number++;
        }
        gragas->acceleration.x = 0;
    }
}

void goto_barrel(gragas_t *gragas, barrel_t *barrel, float rawDt)
{
    gragas->acceleration.x = 0;
    if (barrel != NULL) {
        while (barrel->next_barrel != NULL && barrel->dead)
            barrel = barrel->next_barrel;
        if (!barrel->at_floor)
            return;
        walking_animation(gragas, rawDt);
        if (gragas->rect.x < barrel->rect.x) {
            gragas->acceleration.x = 20;
        } if (gragas->rect.x > barrel->rect.x) {
            gragas->acceleration.x -= 20;
        }
        touching_barrel(gragas, barrel);
    }
}
