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

    // NOTE: HideCursor() breaks GetMousePosition() on the web/Emscripten
    // platform specifically -- rcore_web.c's MouseCursorPosCallback only
    // updates the absolute cursor position when cursorHidden is FALSE;
    // once hidden it silently switches to a relative-movement/pointer-
    // lock model that never gets properly engaged here, leaving
    // GetMousePosition() stuck returning whatever it last had (usually
    // (0,0)) forever. This is exactly why the in-game crosshair sprite
    // didn't track the real cursor. Desktop's GLFW backend has no such
    // issue (GLFW_CURSOR_HIDDEN still tracks absolute position
    // normally), so only call it there; the web build instead hides the
    // system cursor purely via CSS (see web/shell.html's `cursor: none`
    // on the canvas), which has no effect on input tracking.
#if !defined(PLATFORM_WEB)
    HideCursor();
#endif
    SetTargetFPS(FPS);
}
