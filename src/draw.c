/*
** EPITECH PROJECT, 2022 (raylib port)
** draw -- ported 1:1. sfRenderWindow_drawX calls become draw_sprite/
** draw_shadow/DrawTextEx calls; sfRenderWindow_display is handled by
** main.c's BeginDrawing/EndDrawing wrapper instead of here.
*/
#include "my.h"

static void draw_background(bg_t *bg)
{
    draw_sprite(&bg->sky);
    draw_sprite(&bg->clouds);
    draw_sprite(&bg->mountain);
    draw_sprite(&bg->far_woods);
}

void draw_barrels(barrel_t *barrel, bool spawning, int barrel_count)
{
    if (barrel_count == 0 || barrel == NULL)
        return;
    do {
        if (!(spawning ^ barrel->spawning) && !barrel->dead)
            draw_shadow(&barrel->shadow);
        if (!(spawning ^ barrel->spawning))
            draw_sprite(&barrel->sp);
        barrel = barrel->next_barrel;
    } while (barrel != NULL);
}

static void draw_text_int(textint_t *t, Font font, bool hasFont)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s%d", t->string, t->number);
    if (hasFont)
        DrawTextEx(font, buf, t->pos, 32, 1, WHITE);
    else
        DrawText(buf, (int)t->pos.x, (int)t->pos.y, 32, WHITE);
}

void draw_on_screen(game_t *g)
{
    draw_background(g->bg);
    if (g->in_menu)
        draw_sprite(&g->start_button);
    draw_barrels(g->barrel, true, g->barrel_count);
    draw_sprite(&g->bg->tiles);
    draw_barrels(g->barrel, false, g->barrel_count);
    if (g->gragas != NULL) {
        draw_shadow(&g->gragas->shadow);
        draw_sprite(&g->gragas->sp);
        draw_text_int(g->gragas->gscore, g->font, g->hasFont);
    }
    draw_text_int(g->score, g->font, g->hasFont);
    draw_text_int(g->round, g->font, g->hasFont);
    draw_sprite(&g->bg->tree);
    draw_sprite(&g->bg->front_grass);
    draw_sprite(&g->sight);
}
