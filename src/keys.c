/*
** EPITECH PROJECT, 2022 (raylib port)
** keys -- ported 1:1. key_code is our own small marker (1=Enter,
** 2=Space) set by event_handler.c's raylib_key_to_original, since
** raylib's KEY_* enum doesn't match SFML's sfKeyCode values.
*/
#include "my.h"

#define KEY_MARKER_ENTER 1
#define KEY_MARKER_SPACE 2

void manage_keys(game_t *g, int key_code)
{
    if (key_code == KEY_MARKER_ENTER) {
        spawn_barrel(g, 1);
    } if (key_code == KEY_MARKER_SPACE) {
        if (g->gragas == NULL) {
            g->gragas = malloc(sizeof(gragas_t));
            create_gragas(g->gragas, g->textures->gragas);
        }
    }
    manage_mouse_click(g);
}
