/*
** EPITECH PROJECT, 2022 (raylib port)
** rounds -- ported 1:1. round number lives in g->round->number, exactly
** like the original (the "Round: " textint_t doubles as the round
** counter there too).
*/
#include "my.h"

static void get_round_settings(game_t *g, float *spawn_rate, int *barrel_health, float rawDt)
{
    switch (g->round->number) {
    case 1:
        g->round->number += rd_1(spawn_rate, barrel_health, g->barrels_spawned);
        break;
    case 2:
        g->round->number += rd_2(spawn_rate, barrel_health, g->barrels_spawned);
        break;
    case 3:
        g->round->number += rd_3(spawn_rate, barrel_health, g->barrels_spawned);
        break;
    case 4:
        g->round->number += rd_4(spawn_rate, barrel_health, g->barrels_spawned);
        break;
    case 5:
        g->round->number += rd_5(spawn_rate, barrel_health, g, rawDt);
        break;
    }
}

void spawn_round(game_t *g, float rawDt)
{
    g->round_clock += rawDt;
    float time = g->round_clock;
    float spawn_rate = 1;
    int barrel_health = 1;
    if (g->in_menu)
        return;
    get_round_settings(g, &spawn_rate, &barrel_health, rawDt);

    if (g->round->number == 2 && g->gragas == NULL) {
        g->gragas = malloc(sizeof(gragas_t));
        create_gragas(g->gragas, g->textures->gragas);
    } if (g->gragas != NULL) {
        spawn_rate -= g->gragas->gscore->number / 30.0f;
        if (spawn_rate < 0.3f)
            spawn_rate = 0.3f;
    } if (time > 1 / spawn_rate && g->barrels_spawned < 70) {
        spawn_barrel(g, barrel_health);
        g->round_clock = 0;
    }
}
