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
    // rounding both X positions to the nearest HUNDRED pixels. That's a
    // very coarse bucket -- two positions up to ~90px apart can still
    // round to the same hundred and register as a "touch", letting
    // Gragas kill a barrel well before he's actually next to it. That's
    // exactly what reads as the explosion "playing like it was shot
    // very far": the explosion animation plays at the barrel's real
    // (still distant) position while Gragas is still visibly
    // approaching. Replaced with an actual proximity check tight
    // enough to require them to be genuinely close/overlapping,
    // matching how player clicks already require literal overlap
    // (frect_contains) in barrel_touch.c.
    if (ABS(gragas->rect.x - barrel->rect.x) < gragas->rect.width) {
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
