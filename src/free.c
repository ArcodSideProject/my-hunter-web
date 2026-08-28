/*
** EPITECH PROJECT, 2022 (raylib port)
** free -- ported 1:1 in structure; sfXDestroy calls become UnloadTexture/
** UnloadImage/UnloadFont/CloseWindow calls where a raylib resource
** actually needs releasing (this port's rsprite_t/rshape_t/textint_t
** hold no separate heap-allocated CSFML objects beyond the plain
** malloc'd string, so most of the original's per-object destroy calls
** simply don't apply here and are omitted, not silently dropped).
*/
#include "my.h"

void free_background(bg_t *bg)
{
    UnloadImage(bg->tree_image);
    free(bg);
}

void free_barrels(barrel_t *barrel, int barrel_count)
{
    if (barrel_count <= 0 || barrel == NULL)
        return;
    if (barrel->next_barrel != NULL)
        free_barrels(barrel->next_barrel, barrel_count);
    free_barrel(barrel);
}

void free_gragas(gragas_t *gragas)
{
    if (gragas == NULL)
        return;
    free(gragas->gscore->string);
    free(gragas->gscore);
    free(gragas);
}

static void free_texts(game_t *g)
{
    free(g->score->string);
    free(g->score);
    free(g->round->string);
    free(g->round);
}

static void free_textures(textures_t *textures)
{
    UnloadTexture(textures->start_button);
    UnloadTexture(textures->barrel);
    UnloadTexture(textures->explosion);
    UnloadTexture(textures->sight);
    UnloadTexture(textures->gragas);
    UnloadTexture(textures->sky);
    UnloadTexture(textures->clouds);
    UnloadTexture(textures->mountain);
    UnloadTexture(textures->far_woods);
    UnloadTexture(textures->tiles);
    UnloadTexture(textures->tree);
    UnloadTexture(textures->front_grass);
    free(textures);
}

void big_free(game_t *g)
{
    free_background(g->bg);
    free_barrels(g->barrel, g->barrel_count);
    free_gragas(g->gragas);
    free_texts(g);
    if (g->hasFont)
        UnloadFont(g->font);
    free_textures(g->textures);
}
