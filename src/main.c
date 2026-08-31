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

// Scoreboard UI layout: a card centered above the Play Again button,
// holding either the pseudo-entry field or the best/tries readout plus
// "Change name" / "Scoreboard" buttons.
static Rectangle ScoreboardCardRect(void)
{
    float w = 360, h = 190;
    return (Rectangle){ WIDTH / 2.0f - w / 2, HEIGHT * 0.18f, w, h };
}

static Rectangle PseudoFieldRect(Rectangle card)
{
    return (Rectangle){ card.x + 20, card.y + 55, card.width - 40, 40 };
}

static Rectangle SubmitNameButtonRect(Rectangle card)
{
    return (Rectangle){ card.x + card.width / 2 - 70, card.y + 110, 140, 40 };
}

static Rectangle ChangeNameButtonRect(Rectangle card)
{
    return (Rectangle){ card.x + 15, card.y + card.height - 50, 150, 36 };
}

static Rectangle ScoreboardButtonRect(Rectangle card)
{
    return (Rectangle){ card.x + card.width - 165, card.y + card.height - 50, 150, 36 };
}

static Rectangle CloseScoreboardButtonRect(void)
{
    return (Rectangle){ WIDTH / 2.0f - 60, HEIGHT * 0.85f, 120, 40 };
}

static void SubmitCurrentScore(game_t *g)
{
    if (g->score_submitted) return;
    scoreboard_submit(g->pseudo, g->score->number, &g->last_result);
    g->has_last_result = true;
    g->score_submitted = true;
}

static void DrawScoreboardCard(game_t *g)
{
    Rectangle card = ScoreboardCardRect();
    Font f = g->hasFont ? g->font : GetFontDefault();
    DrawRectangleRounded(card, 0.08f, 8, (Color){20, 20, 25, 235});
    DrawRectangleRoundedLinesEx(card, 0.08f, 8, 2, (Color){90, 90, 100, 255});

    if (g->editing_pseudo) {
        const char *title = "Enter your name";
        Vector2 ts = MeasureTextEx(f, title, 22, 1);
        DrawTextEx(f, title, (Vector2){card.x + card.width / 2 - ts.x / 2, card.y + 14}, 22, 1, RAYWHITE);

        Rectangle field = PseudoFieldRect(card);
        DrawRectangleRounded(field, 0.2f, 6, (Color){35, 35, 42, 255});
        DrawRectangleRoundedLinesEx(field, 0.2f, 6, 2, (Color){130, 130, 145, 255});
        text_input_update(g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
        char display[SCOREBOARD_NAME_MAX + 2];
        snprintf(display, sizeof(display), "%s%s", g->pseudo_edit_buf,
                 ((int)(GetTime() * 2) % 2 == 0) ? "|" : "");
        DrawTextEx(f, display, (Vector2){field.x + 10, field.y + 9}, 22, 1, RAYWHITE);

        Rectangle btn = SubmitNameButtonRect(card);
        bool valid = g->pseudo_edit_buf[0] != '\0';
        Color btnColor = valid ?
            (CheckCollisionPointRec(g->mpos, btn) ? (Color){80, 160, 90, 255} : (Color){60, 130, 70, 255})
            : (Color){60, 60, 65, 255};
        DrawRectangleRounded(btn, 0.25f, 8, btnColor);
        const char *label = "Confirm";
        Vector2 ls = MeasureTextEx(f, label, 20, 1);
        DrawTextEx(f, label, (Vector2){btn.x + btn.width / 2 - ls.x / 2, btn.y + btn.height / 2 - ls.y / 2}, 20, 1, RAYWHITE);

        if (valid && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(g->mpos, btn)) {
            strncpy(g->pseudo, g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
            g->pseudo[SCOREBOARD_NAME_MAX] = '\0';
            scoreboard_save_pseudo(g->pseudo);
            g->editing_pseudo = false;
            g->score_submitted = false; // allow (re-)submission under the new/confirmed name
            SubmitCurrentScore(g);
        }
        return;
    }

    if (!g->has_last_result) SubmitCurrentScore(g);

    char nameLine[SCOREBOARD_NAME_MAX + 16];
    snprintf(nameLine, sizeof(nameLine), "%s", g->pseudo);
    Vector2 ns = MeasureTextEx(f, nameLine, 24, 1);
    DrawTextEx(f, nameLine, (Vector2){card.x + card.width / 2 - ns.x / 2, card.y + 14}, 24, 1, RAYWHITE);

    char statsLine[64];
    if (g->has_last_result && g->last_result.ok) {
        snprintf(statsLine, sizeof(statsLine), "Best: %d   Tries: %d", g->last_result.best, g->last_result.tries);
    } else {
        snprintf(statsLine, sizeof(statsLine), "Score: %d (offline)", g->score->number);
    }
    Vector2 ss = MeasureTextEx(f, statsLine, 20, 1);
    DrawTextEx(f, statsLine, (Vector2){card.x + card.width / 2 - ss.x / 2, card.y + 50}, 20, 1, (Color){220, 220, 220, 255});

    Rectangle changeBtn = ChangeNameButtonRect(card);
    Color changeColor = CheckCollisionPointRec(g->mpos, changeBtn) ? (Color){90, 90, 100, 255} : (Color){60, 60, 70, 255};
    DrawRectangleRounded(changeBtn, 0.25f, 8, changeColor);
    Vector2 cs = MeasureTextEx(f, "Change name", 16, 1);
    DrawTextEx(f, "Change name", (Vector2){changeBtn.x + changeBtn.width / 2 - cs.x / 2, changeBtn.y + changeBtn.height / 2 - cs.y / 2}, 16, 1, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(g->mpos, changeBtn)) {
        strncpy(g->pseudo_edit_buf, g->pseudo, SCOREBOARD_NAME_MAX);
        g->pseudo_edit_buf[SCOREBOARD_NAME_MAX] = '\0';
        g->editing_pseudo = true;
    }

    Rectangle boardBtn = ScoreboardButtonRect(card);
    Color boardColor = CheckCollisionPointRec(g->mpos, boardBtn) ? (Color){90, 90, 100, 255} : (Color){60, 60, 70, 255};
    DrawRectangleRounded(boardBtn, 0.25f, 8, boardColor);
    Vector2 bs = MeasureTextEx(f, "Scoreboard", 16, 1);
    DrawTextEx(f, "Scoreboard", (Vector2){boardBtn.x + boardBtn.width / 2 - bs.x / 2, boardBtn.y + boardBtn.height / 2 - bs.y / 2}, 16, 1, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(g->mpos, boardBtn)) {
        g->board_count = scoreboard_fetch_all(g->board);
        g->board_loaded = true;
        g->show_scoreboard = true;
    }
}

static void DrawScoreboardOverlay(game_t *g)
{
    DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 200});
    Font f = g->hasFont ? g->font : GetFontDefault();

    float listW = 420;
    float listX = WIDTH / 2.0f - listW / 2;
    float top = HEIGHT * 0.12f;

    const char *title = "Scoreboard";
    Vector2 ts = MeasureTextEx(f, title, 28, 1);
    DrawTextEx(f, title, (Vector2){WIDTH / 2.0f - ts.x / 2, top}, 28, 1, RAYWHITE);

    float rowY = top + 50;
    float rowH = 26;
    if (g->board_count == 0) {
        DrawTextEx(f, "No scores yet.", (Vector2){listX, rowY}, 18, 1, (Color){200, 200, 200, 255});
    }
    for (int i = 0; i < g->board_count && rowY + rowH < HEIGHT * 0.8f; i++) {
        char line[80];
        snprintf(line, sizeof(line), "%2d. %-20s %6d (%d tries)",
                 i + 1, g->board[i].name, g->board[i].best, g->board[i].tries);
        bool isMe = strcmp(g->board[i].name, g->pseudo) == 0;
        DrawTextEx(f, line, (Vector2){listX, rowY}, 18, 1, isMe ? (Color){255, 210, 90, 255} : RAYWHITE);
        rowY += rowH;
    }

    Rectangle closeBtn = CloseScoreboardButtonRect();
    Color closeColor = CheckCollisionPointRec(g->mpos, closeBtn) ? (Color){160, 70, 60, 255} : (Color){120, 50, 45, 255};
    DrawRectangleRounded(closeBtn, 0.25f, 8, closeColor);
    Vector2 cs = MeasureTextEx(f, "Close", 20, 1);
    DrawTextEx(f, "Close", (Vector2){closeBtn.x + closeBtn.width / 2 - cs.x / 2, closeBtn.y + closeBtn.height / 2 - cs.y / 2}, 20, 1, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(g->mpos, closeBtn)) {
        g->show_scoreboard = false;
    }
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
    // texture handles if called again wholesale). Scoreboard state
    // (pseudo, cached board, submission flags) intentionally survives
    // a reset -- only actual gameplay state is cleared.
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
    g->score_submitted = false;
    g->has_last_result = false;
    g->show_scoreboard = false;
    create_texts(g);
}

static void frame(void)
{
    float rawDt = GetFrameTime();
    get_delta_t(&g, rawDt);
    g.mpos = GetMousePosition();

    if (g.game_over) {
        if (!g.show_scoreboard && !g.editing_pseudo &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
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
        DrawScoreboardCard(&g);
        if (g.show_scoreboard) DrawScoreboardOverlay(&g);
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

    bool hadSavedPseudo = scoreboard_load_saved_pseudo(g.pseudo);
    g.editing_pseudo = !hadSavedPseudo; // first-ever run: land straight in the name field

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
