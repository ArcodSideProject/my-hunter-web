/*
** EPITECH PROJECT, 2022 (raylib port)
** mouse -- ported 1:1
*/
#include "my.h"

void move_sight_to_cursor(game_t *g)
{
    v2f pos;
    frect_t bounds = sprite_get_global_bounds(&g->sight);
    pos.x = g->mpos.x - bounds.width / 2;
    pos.y = g->mpos.y - bounds.height / 2;
    sprite_set_position(&g->sight, pos);
}

void manage_mouse_click(game_t *g)
{
    if (g->in_menu) {
        frect_t start_btn_rect = sprite_get_global_bounds(&g->start_button);
        if (frect_contains(&start_btn_rect, (int)g->mpos.x, (int)g->mpos.y)) {
            g->round->number = 1;
            g->in_menu = false;
        }
    }
    if (g->barrel_count > 0)
        for_touched_barrels(g->barrel, g->mpos);
    if (g->gragas)
        gragas_touch(g->gragas, g->mpos);
}
