/*
** EPITECH PROJECT, 2022 (raylib port)
** bounce
** File description:
** bounce -- unchanged from the original's logic
*/
#include "my.h"

void bounce_on_border(barrel_t *barrel, float rawDt)
{
    (void)rawDt;
    int floor_pos = HEIGHT - (int)barrel->rect.height - FLOOR_HEIGHT;
    if (barrel->rect.x < 0)
        barrel->velocity.x = ABS(barrel->velocity.x);
    if (barrel->rect.x > WIDTH - barrel->rect.width)
        barrel->velocity.x = -ABS(barrel->velocity.x);
    if (barrel->spawning)
        return;
    barrel->at_floor = false;
    if (barrel->rect.y > floor_pos) {
        if (barrel->velocity.y <= GRAVITY && barrel->velocity.y > 0) {
            barrel->at_floor = true;
            barrel->velocity.y = 0;
            return;
        }
        barrel->velocity = int_multiply_v2f(barrel->velocity, FLOOR_FRICTION);
        barrel->velocity.y = -ABS(barrel->velocity.y);
    }
}
