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

// Scoreboard "final wave" name label (explicit user request): a small
// pill with a semi-transparent background, drawn just above the
// barrel, following it every frame since it's positioned from the
// barrel's own live global bounds -- no separate tracking needed.
static void draw_scoreboard_name_label(barrel_t *barrel, Font font, bool hasFont)
{
    if (barrel->scoreboard_name[0] == '\0' || barrel->dead)
        return;
    Font f = hasFont ? font : GetFontDefault();
    int fontSize = 18;
    Vector2 size = MeasureTextEx(f, barrel->scoreboard_name, fontSize, 1);
    float padX = 8, padY = 4;
    Rectangle bg = {
        barrel->rect.x + barrel->rect.width / 2 - size.x / 2 - padX,
        barrel->rect.y - size.y - padY * 2 - 4,
        size.x + padX * 2,
        size.y + padY * 2
    };
    DrawRectangleRounded(bg, 0.3f, 6, (Color){0, 0, 0, 140});
    Vector2 textPos = { bg.x + padX, bg.y + padY };
    if (hasFont)
        DrawTextEx(f, barrel->scoreboard_name, textPos, fontSize, 1, RAYWHITE);
    else
        DrawText(barrel->scoreboard_name, (int)textPos.x, (int)textPos.y, fontSize, RAYWHITE);
}

void draw_barrels(barrel_t *barrel, bool spawning, int barrel_count, Font font, bool hasFont)
{
    if (barrel_count == 0 || barrel == NULL)
        return;
    do {
        if (!(spawning ^ barrel->spawning) && !barrel->dead)
            draw_shadow(&barrel->shadow);
        if (!(spawning ^ barrel->spawning)) {
            draw_sprite(&barrel->sp);
            draw_scoreboard_name_label(barrel, font, hasFont);
        }
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

// Barrels-left counter (explicit user request, not in the original):
// how many more barrels this round still has to throw. barrels_spawned
// is cumulative across the whole run (never reset between rounds --
// see easy_rounds.c's per-round thresholds, which are all checked
// against that same running total), so "left" is simply this round's
// threshold minus barrels_spawned, floored at 0. Round 5 also folds in
// the scoreboard final-wave queue once it's started, since those
// barrels are additional to the normal 70-barrel count.
static int barrels_left_this_round(game_t *g)
{
    int threshold;
    switch (g->round->number) {
    case 1: threshold = 5; break;
    case 2: threshold = 15; break;
    case 3: threshold = 30; break;
    case 4: threshold = 33; break;
    case 5: threshold = 70; break;
    default: return 0;
    }
    int left = threshold - g->barrels_spawned;
    if (left < 0) left = 0;
    if (g->round->number == 5 && g->final_wave_started) {
        int queueLeft = g->final_wave_queue_count - g->final_wave_queue_next;
        if (queueLeft < 0) queueLeft = 0;
        left += queueLeft;
    }
    return left;
}

static void draw_barrels_left(game_t *g)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Barrels left: %d", barrels_left_this_round(g));
    Font f = g->hasFont ? g->font : GetFontDefault();
    Vector2 pos = { 20, 80 };
    if (g->hasFont)
        DrawTextEx(f, buf, pos, 24, 1, (Color){230, 230, 230, 255});
    else
        DrawText(buf, (int)pos.x, (int)pos.y, 24, (Color){230, 230, 230, 255});
}

static void draw_enter_hint(game_t *g)
{
    // Per explicit request: "PRESS ENTER" positioned behind the tree,
    // only really readable once the player hovers the mouse over the
    // tree and its transparency drops (blur_tree.c fades it from fully
    // opaque 255 down to 150 while hovered). Drawn in the gameplay pass
    // (BEFORE the tree, in z-order) so the tree sprite naturally
    // occludes/reveals it depending on its current transparency,
    // instead of always floating on top of everything.
    const char *hint = "PRESS ENTER";
    int fontSize = 28;
    float treeX = TREE_OFFSET_X * g->resize.x;
    float treeY = TREE_OFFSET_Y * g->resize.y;
    float treeW = 113 * g->resize.x;
    float treeH = 123 * g->resize.y;
    Font f = g->hasFont ? g->font : GetFontDefault();
    Vector2 size = MeasureTextEx(f, hint, fontSize, 1);
    Vector2 pos = { treeX + treeW / 2 - size.x / 2, treeY + treeH / 2 - size.y / 2 };
    // fade the hint's own alpha in step with the tree's transparency:
    // tree_transparency ranges 150 (hovered/faded) .. 255 (opaque), so
    // remap that range to a full 0..255 alpha instead of the raw
    // 255-transparency value (which would only ever reach ~105/255 at
    // most and look too dim even at maximum fade).
    float fadeAmount = (255.0f - (float)g->bg->tree_transparency) / (255.0f - 150.0f);
    if (fadeAmount < 0) fadeAmount = 0;
    if (fadeAmount > 1) fadeAmount = 1;
    unsigned char alpha = (unsigned char)(fadeAmount * 255.0f);
    if (g->hasFont) {
        DrawTextEx(f, hint, pos, fontSize, 1, (Color){255, 220, 120, alpha});
    } else {
        DrawText(hint, (int)pos.x, (int)pos.y, fontSize, (Color){255, 220, 120, alpha});
    }
}

void draw_on_screen_background_and_gameplay(game_t *g)
{
    draw_background(g->bg);
    if (g->in_menu)
        draw_sprite(&g->start_button);
    draw_barrels(g->barrel, true, g->barrel_count, g->font, g->hasFont);
    draw_sprite(&g->bg->tiles);
    draw_barrels(g->barrel, false, g->barrel_count, g->font, g->hasFont);
    if (g->gragas != NULL) {
        draw_shadow(&g->gragas->shadow);
        draw_sprite(&g->gragas->sp);
        draw_text_int(g->gragas->gscore, g->font, g->hasFont);
    }
    draw_text_int(g->score, g->font, g->hasFont);
    draw_text_int(g->round, g->font, g->hasFont);
    if (!g->in_menu && !g->game_over) {
        draw_barrels_left(g);
        draw_enter_hint(g);
    }
}

void draw_on_screen_foreground(game_t *g)
{
    draw_sprite(&g->bg->tree);
    draw_sprite(&g->bg->front_grass);
    draw_sprite(&g->sight);
}

void draw_on_screen(game_t *g)
{
    draw_on_screen_background_and_gameplay(g);
    draw_on_screen_foreground(g);
}
