/*
** EPITECH PROJECT, 2022 (raylib port)
** initialize_game -- ported 1:1, minus window creation (main.c calls
** InitWindow itself, before this runs, since raylib's window must
** exist before textures can be loaded).
*/
#include "my.h"

static void load_textures(textures_t *textures)
{
    textures->start_button = LoadTexture("assets/start_button.png");
    textures->barrel = LoadTexture("assets/barrel.png");
    textures->explosion = LoadTexture("assets/explosion.png");
    textures->sight = LoadTexture("assets/sights/red.png");
    textures->gragas = LoadTexture("assets/sumos.png");

    textures->sky = LoadTexture("assets/background/Sky.png");
    textures->clouds = LoadTexture("assets/background/Clouds.png");
    textures->mountain = LoadTexture("assets/background/Mountain.png");
    textures->far_woods = LoadTexture("assets/background/Far_woods.png");
    textures->tiles = LoadTexture("assets/background/Tiles.png");
    textures->tree = LoadTexture("assets/background/Tree.png");
    textures->front_grass = LoadTexture("assets/background/Front_grass.png");
}

void initialize_game(game_t *g)
{
    g->in_menu = true;
    g->game_over = false;
    g->textures = malloc(sizeof(textures_t));
    load_textures(g->textures);
    g->start_button = create_start_button(g->textures->start_button);
    g->bg = malloc(sizeof(bg_t));
    g->resize = create_background(g->bg, g->textures);
    g->sight = create_sprite(g->textures->sight, (v2f){0.7f, 0.7f});
    g->font = LoadFontEx("assets/fonts/upheavtt.ttf", 64, NULL, 0);
    g->hasFont = (g->font.texture.id != 0);
    if (!g->hasFont)
        g->font = GetFontDefault();
    create_texts(g);
    g->barrel_count = 0;
    g->barrels_spawned = 0;
    g->barrel = NULL;
    g->gragas = NULL;
    g->round_clock = 0;
    g->game_dt = 0;
    g->dt = 0;

    HideCursor();
    SetTargetFPS(FPS);
}
