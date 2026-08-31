/*
** EPITECH PROJECT, 2022 (raylib port)
** spawn_barrel -- ported 1:1 (linked list preserved, not converted to
** an array, to stay structurally faithful to the original)
*/
#include "my.h"

static void send_barrel(barrel_t **barrel, int *barrel_count,
textures_t *textures, int max_health)
{
    if (*barrel_count == 1) {
        (*barrel) = malloc(sizeof(barrel_t));
        create_barrel((*barrel), max_health, textures->barrel,
        textures->explosion);
        return;
    }
    while ((*barrel)->next_barrel != NULL) {
        barrel = &(*barrel)->next_barrel;
    }
    (*barrel)->next_barrel = malloc(sizeof(barrel_t));
    create_barrel((*barrel)->next_barrel, max_health, textures->barrel,
    textures->explosion);
}

void spawn_barrel(game_t *g, int max_health)
{
    g->barrel_count++;
    g->barrels_spawned++;
    send_barrel(&g->barrel, &g->barrel_count, g->textures, max_health);
}

// Scoreboard "final wave" (explicit user request, not in the original):
// right as round 5's natural barrel supply runs out, spawn one extra
// barrel per known scoreboard entry, with health set to that player's
// try count (more tries = tougher barrel) and its name attached so
// draw.c can render a following label. Guarded by g->final_wave_spawned
// so this only ever fires once per round.
void spawn_scoreboard_final_wave(game_t *g)
{
    if (g->final_wave_spawned)
        return;
    g->final_wave_spawned = true;

    scoreboard_entry_t entries[SCOREBOARD_MAX_ENTRIES];
    int count = scoreboard_fetch_all(entries);

    for (int i = 0; i < count; i++) {
        int health = entries[i].tries > 0 ? entries[i].tries : 1;
        if (health > 50) health = 50; // sanity cap -- a huge tries count
                                       // shouldn't make one barrel
                                       // effectively unkillable
        spawn_barrel(g, health);

        // The barrel we just appended is the new tail of the list.
        barrel_t *tail = g->barrel;
        while (tail->next_barrel != NULL) tail = tail->next_barrel;
        tail->scoreboard_name = malloc(strlen(entries[i].name) + 1);
        if (tail->scoreboard_name != NULL)
            strcpy(tail->scoreboard_name, entries[i].name);
    }
}
