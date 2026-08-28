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

static void draw_enter_hint(game_t *g)
{
    // Explicit request: a visible hint that holding ENTER spawns more
    // barrels. Placed bottom-left, small and unobtrusive, only during
    // actual gameplay (not the menu or the game-over screen, which
    // have their own separate messaging).
    const char *hint = "Hold ENTER to spawn more barrels";
    int fontSize = 20;
    Vector2 pos = {16, HEIGHT - 60};
    if (g->hasFont) {
        DrawTextEx(g->font, hint, pos, fontSize, 1, (Color){255, 255, 255, 200});
    } else {
        DrawText(hint, (int)pos.x, (int)pos.y, fontSize, (Color){255, 255, 255, 200});
    }
}

void draw_on_screen_background_and_gameplay(game_t *g)
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
}

void draw_on_screen_foreground(game_t *g)
{
    draw_sprite(&g->bg->tree);
    draw_sprite(&g->bg->front_grass);
    // Drawn AFTER front_grass (not before, in the gameplay pass where
    // it was originally placed) -- front_grass is a full-width opaque
    // strip covering roughly the bottom ~13% of the screen, which was
    // completely painting over the hint text at its old position.
    if (!g->in_menu && !g->game_over)
        draw_enter_hint(g);
    draw_sprite(&g->sight);
}

void draw_on_screen(game_t *g)
{
    draw_on_screen_background_and_gameplay(g);
    draw_on_screen_foreground(g);
}
