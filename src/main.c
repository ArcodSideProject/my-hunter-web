/*
** EPITECH PROJECT, 2022 (raylib port)
** main -- ported 1:1 from render_window()/main(); window creation and
** the main-loop driver are adapted for raylib (InitWindow +
** BeginDrawing/EndDrawing instead of sfRenderWindow_create/display),
** and for the web build, emscripten_set_main_loop instead of a plain
** while loop, since the browser owns the frame timing there.
*/
#include "my.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static game_t g;

static void frame(void)
{
    float rawDt = GetFrameTime();
    get_delta_t(&g, rawDt);
    g.mpos = GetMousePosition();
    spawn_round(&g, g.game_dt);

    event_handler(&g);

    remove_dead_barrels(&g);
    animate_barrels(g.barrel_count, g.barrel, g.dt, g.game_dt);
    animate_gragas(g.gragas, g.dt, g.game_dt, g.barrel);
    move_sight_to_cursor(&g);
    blur_tree(&g);

    update_texts(&g);

    move_clouds(&g.bg->clouds, &g.bg->clouds_speed,
    g.gragas == NULL ? 0 : g.gragas->gscore->number);

    BeginDrawing();
    ClearBackground(BLACK);
    draw_on_screen(&g);
    if (g.game_over) {
        DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 180});
        const char *msg = "Cleared! Thanks for playing.";
        int w = MeasureText(msg, 40);
        DrawText(msg, WIDTH / 2 - w / 2, HEIGHT / 2 - 20, 40, GOLD);
    }
    EndDrawing();
}

int main(void)
{
    srand((unsigned int)time(NULL));

    InitWindow(WIDTH, HEIGHT, "My Hunter");
    memset(&g, 0, sizeof(g));
    initialize_game(&g);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (!g.game_over && !WindowShouldClose()) {
        frame();
    }
#endif

    big_free(&g);
    CloseWindow();
    return 0;
}
