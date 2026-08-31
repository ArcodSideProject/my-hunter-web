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
    // NOTE: natural spawning in spawn_round() is gated by
    // "barrels_spawned < 70", so the counter can reach at most exactly
    // 70 without an off-path spawn (e.g. the ENTER key in keys.c, which
    // calls spawn_barrel() directly, bypassing that gate). The original
    // ">" check here meant the round could only ever end if a player
    // pressed ENTER to push the count past 70 -- impossible on
    // touch-only platforms (mobile), where the round could never
    // finish on its own. Using ">=" makes the natural spawn cap alone
    // sufficient to trigger the win/end condition.
    if (g->barrels_spawned >= 70 && g->barrel_count == 0) {
        // Scoreboard "final wave" (explicit user request): before
        // actually ending the round, spawn one extra barrel per known
        // scoreboard entry. spawn_scoreboard_final_wave() bumps
        // barrels_spawned/barrel_count itself via spawn_barrel(), so
        // this same condition naturally re-evaluates false until that
        // wave is cleared too -- the round only truly ends once every
        // scoreboard barrel is dead as well. Guarded internally so it
        // only ever runs once.
        if (!g->final_wave_spawned) {
            spawn_scoreboard_final_wave(g);
            return 0;
        }
        g->game_over = true;
        return 1;
    }
    return 0;
}
