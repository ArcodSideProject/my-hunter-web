/*
** EPITECH PROJECT, 2022 (raylib port)
** get_delta_t -- ported 1:1 in spirit. The original measured elapsed
** time via an sfClock and computed a delta from consecutive reads;
** raylib's GetFrameTime() already gives us the real per-frame delta
** directly, so rawDt is that value, and g->dt is the same GAME_TICK
** scaling the original applied.
*/
#include "my.h"

void get_delta_t(game_t *g, float rawDt)
{
    g->game_dt = rawDt;
    g->dt = rawDt * GAME_TICK;
}
