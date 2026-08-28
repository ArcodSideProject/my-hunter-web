/*
** EPITECH PROJECT, 2022 (raylib port)
** easy_rounds -- ported 1:1
*/
#include "my.h"

int rd_1(float *spawn_rate, int *barrel_health, int barrels_spawned)
{
    (*spawn_rate) = 0.5f;
    (*barrel_health) = 1;
    if (barrels_spawned > 5)
        return 1;
    return 0;
}

int rd_2(float *spawn_rate, int *barrel_health, int barrels_spawned)
{
    (*spawn_rate) = 1;
    (*barrel_health) = 1;
    if (barrels_spawned > 15)
        return 1;
    return 0;
}

int rd_3(float *spawn_rate, int *barrel_health, int barrels_spawned)
{
    (*spawn_rate) = 0.5f;
    (*barrel_health) = 3;
    if (barrels_spawned > 30)
        return 1;
    return 0;
}

int rd_4(float *spawn_rate, int *barrel_health, int barrels_spawned)
{
    (*spawn_rate) = 0.4f;
    (*barrel_health) = 10;
    if (barrels_spawned > 33)
        return 1;
    return 0;
}

int rd_5(float *spawn_rate, int *barrel_health, game_t *g)
{
    (*spawn_rate) = 6;
    (*barrel_health) = 1 + rand() % 2;
    if (g->barrels_spawned > 70 && g->barrel_count == 0) {
        g->game_over = true;
        return 1;
    }
    return 0;
}
