/*
** EPITECH PROJECT, 2022 (raylib port)
** goto_barrel -- ported 1:1
*/
#include "my.h"

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
    // NOTE: the original used my_round(gragas->rect.x)==my_round(barrel->rect.x),
    // rounding both X positions to the nearest HUNDRED pixels, letting
    // Gragas kill a barrel up to ~90px away. A first fix attempt
    // replaced that with `< gragas->rect.width` (his full sprite
    // bounding box, 100px) as the threshold -- but that's still far
    // too loose: real measured gaps at kill time were consistently
    // 90-99px, i.e. Gragas could still pop a barrel that's visually a
    // full body-width away from him, which is exactly what read as
    // "barrel goes nonsense when exploding". Tightened to a genuinely
    // small, visually-tight proximity (30px) so the kill only fires
    // once he's actually standing right next to the barrel.
    if (ABS(gragas->rect.x - barrel->rect.x) < 30.0f) {
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
