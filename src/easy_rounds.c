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

int rd_5(float *spawn_rate, int *barrel_health, game_t *g, float rawDt)
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
    //
    // Once the natural supply is exhausted, the scoreboard "final wave"
    // (explicit user request) takes over: it trickles in one barrel
    // per known scoreboard entry at a high rate (not all at once), so
    // it must keep being ticked every frame regardless of
    // barrel_count -- spawning the first queued barrel would otherwise
    // make barrel_count>0 and prevent this branch from running again
    // before the round could ever finish spawning the rest of the
    // queue. Once final_wave_started (queue fetched) but not yet
    // final_wave_done (still spawning entries in), always tick,
    // independent of barrel_count.
    if (g->final_wave_started && !g->final_wave_done) {
        spawn_scoreboard_final_wave_tick(g, rawDt);
        return 0;
    }
    if (g->barrels_spawned >= 70 && g->barrel_count == 0) {
        if (!g->final_wave_started) {
            spawn_scoreboard_final_wave_tick(g, rawDt);
            return 0;
        }
        // final_wave_started && final_wave_done && barrel_count==0:
        // the whole wave has both fully spawned and been cleared.
        g->game_over = true;
        return 1;
    }
    return 0;
}
