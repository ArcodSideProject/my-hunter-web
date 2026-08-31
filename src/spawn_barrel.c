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
// once round 5's natural barrel supply runs out, spawn one extra
// barrel per known scoreboard entry (health = that player's try count,
// name attached so draw.c can render a following label) -- but
// trickled in one at a time at a high rate, not all dumped on screen
// simultaneously. Call once per frame while the wave is active;
// internally guarded so the queue is only fetched once
// (final_wave_started) and each queued entry is only spawned once
// (final_wave_queue_next advances monotonically).
#define FINAL_WAVE_SPAWN_INTERVAL 0.12f // seconds between each queued barrel -- "high rate" but still visibly trickling

void spawn_scoreboard_final_wave_tick(game_t *g, float rawDt)
{
    if (!g->final_wave_started) {
        g->final_wave_started = true;
        g->final_wave_queue_count = scoreboard_fetch_all(g->final_wave_queue);
        g->final_wave_queue_next = 0;
        g->final_wave_spawn_timer = 0;
        g->final_wave_done = (g->final_wave_queue_count == 0);
    }
    if (g->final_wave_done)
        return;

    g->final_wave_spawn_timer += rawDt;
    while (g->final_wave_spawn_timer >= FINAL_WAVE_SPAWN_INTERVAL &&
           g->final_wave_queue_next < g->final_wave_queue_count) {
        g->final_wave_spawn_timer -= FINAL_WAVE_SPAWN_INTERVAL;

        scoreboard_entry_t *entry = &g->final_wave_queue[g->final_wave_queue_next];
        int health = entry->tries > 0 ? entry->tries : 1;
        if (health > 50) health = 50; // sanity cap -- a huge tries count
                                       // shouldn't make one barrel
                                       // effectively unkillable
        spawn_barrel(g, health);

        // The barrel we just appended is the new tail of the list.
        barrel_t *tail = g->barrel;
        while (tail->next_barrel != NULL) tail = tail->next_barrel;
        strncpy(tail->scoreboard_name, entry->name, SCOREBOARD_NAME_MAX);
        tail->scoreboard_name[SCOREBOARD_NAME_MAX] = '\0';

        g->final_wave_queue_next++;
    }
    if (g->final_wave_queue_next >= g->final_wave_queue_count)
        g->final_wave_done = true;
}
