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
