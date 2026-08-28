/*
** EPITECH PROJECT, 2022 (raylib port)
** barrel_touch -- ported 1:1
*/
#include "my.h"

static void barrel_touched(barrel_t *barrel)
{
    int round = 5;
    barrel->velocity.y = -60;
    barrel->velocity.x = round * 3 * (-1 * (rand() % 2 - 0.5f));
    barrel->health--;
}

void for_touched_barrels(barrel_t *barrel, Vector2 mpos)
{
    do {
        if (!barrel->dead &&
        frect_contains(&barrel->rect, (int)mpos.x, (int)mpos.y))
            barrel_touched(barrel);
        barrel = barrel->next_barrel;
    } while (barrel != NULL);
}
