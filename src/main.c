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

static Rectangle PlayAgainButtonRect(game_t *g)
{
    // Placed so it's partially hidden behind the tree sprite (tree sits
    // at TREE_OFFSET_X/Y * g.resize, ~113x123 native * resize scale) --
    // per explicit request, not a plain floating text message. Anchored
    // to the tree's horizontal center, vertically overlapping its lower
    // half/trunk area so roughly half the button is visually behind it.
    float treeX = TREE_OFFSET_X * g->resize.x;
    float treeY = TREE_OFFSET_Y * g->resize.y;
    float treeW = 113 * g->resize.x;
    float treeH = 123 * g->resize.y;
    float btnW = 220, btnH = 64;
    return (Rectangle){
        treeX + treeW / 2 - btnW / 2,
        treeY + treeH * 0.55f,
        btnW, btnH
    };
}

static void DrawPlayAgainButton(game_t *g)
{
    Rectangle btn = PlayAgainButtonRect(g);
    Color fill = CheckCollisionPointRec(g->mpos, btn) ?
        (Color){200, 90, 40, 235} : (Color){160, 70, 30, 220};
    DrawRectangleRounded(btn, 0.25f, 8, fill);
    DrawRectangleRoundedLinesEx(btn, 0.25f, 8, 2, (Color){40, 20, 10, 255});
    const char *label = "Play again";
    Font f = g->hasFont ? g->font : GetFontDefault();
    Vector2 size = MeasureTextEx(f, label, 26, 1);
    DrawTextEx(f, label, (Vector2){btn.x + btn.width / 2 - size.x / 2,
               btn.y + btn.height / 2 - size.y / 2}, 26, 1, RAYWHITE);
}

static void ResetGameKeepAssets(game_t *g)
{
    // Tear down gameplay state and rebuild it fresh, without reloading
    // textures/font (initialize_game() would leak the previous run's
    // texture handles if called again wholesale).
    free_barrels(g->barrel, g->barrel_count);
    free_gragas(g->gragas);
    free(g->score->string);
    free(g->score);
    free(g->round->string);
    free(g->round);

    g->barrel = NULL;
    g->barrel_count = 0;
    g->barrels_spawned = 0;
    g->gragas = NULL;
    g->round_clock = 0;
    g->in_menu = true;
    g->game_over = false;
    g->enter_hold_time = 0;
    g->enter_spawn_accum = 0;
    create_texts(g);
}

static void frame(void)
{
    float rawDt = GetFrameTime();
    get_delta_t(&g, rawDt);
    g.mpos = GetMousePosition();

    if (g.game_over) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(g.mpos, PlayAgainButtonRect(&g))) {
            ResetGameKeepAssets(&g);
        }
        move_sight_to_cursor(&g);
        blur_tree(&g);
        move_clouds(&g.bg->clouds, &g.bg->clouds_speed, 0);

        BeginDrawing();
        ClearBackground(BLACK);
        draw_on_screen_background_and_gameplay(&g);
        DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 120});
        DrawPlayAgainButton(&g);       // drawn BEFORE the tree/grass so
                                        // they visually occlude the top
                                        // half of the button, per
                                        // explicit request ("hidden
                                        // halfly behind the tree")
        draw_on_screen_foreground(&g); // tree, front_grass, sight
        EndDrawing();
        return;
    }

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
    // NOTE: no longer exits the loop when g.game_over becomes true --
    // that used to close the window/end the program the instant round 5
    // finished, before the Play Again screen could ever actually be
    // seen or clicked on native builds (it only appeared to work on web
    // because emscripten_set_main_loop ignores this condition entirely).
    while (!WindowShouldClose()) {
        frame();
    }
#endif

    big_free(&g);
    CloseWindow();
    return 0;
}
