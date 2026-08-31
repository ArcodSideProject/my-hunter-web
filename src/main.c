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

// Scoreboard UI layout: the game-over screen IS the scoreboard --
// pseudo row (editable text field + roll button) at the top, live
// board below (highlighting the current pseudo's row), Play Again
// still available (drawn separately, partially behind the tree).
static Rectangle PseudoFieldRect(void)
{
    return (Rectangle){ WIDTH / 2.0f - 160, HEIGHT * 0.07f, 260, 40 };
}

static Rectangle RollButtonRect(void)
{
    Rectangle field = PseudoFieldRect();
    return (Rectangle){ field.x + field.width + 10, field.y, 40, 40 };
}

static void SubmitCurrentScore(game_t *g)
{
    if (g->score_submitted) return;
    scoreboard_submit(g->pseudo, g->score->number, &g->last_result);
    g->has_last_result = true;
    g->score_submitted = true;
    // Live-refresh the board so the just-submitted score shows
    // immediately (explicit user request: editing your name should
    // live-update the scoreboard you're looking at -- the same applies
    // to a fresh submission).
    g->board_count = scoreboard_fetch_all(g->board);
    g->board_loaded = true;
}

// Commits pseudo_edit_buf as the active pseudo: saves it locally,
// re-submits this run's score under the new name (explicit user
// request: entering an already-taken name just makes you that person,
// no ownership/password), and refreshes the board so the change is
// reflected live.
static void CommitPseudo(game_t *g)
{
    if (g->pseudo_edit_buf[0] == '\0') return; // ignore an empty field, keep the previous pseudo
    if (strcmp(g->pseudo, g->pseudo_edit_buf) == 0 && g->has_last_result)
        return; // no actual change
    strncpy(g->pseudo, g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
    g->pseudo[SCOREBOARD_NAME_MAX] = '\0';
    scoreboard_save_pseudo(g->pseudo);
    g->score_submitted = false;
    SubmitCurrentScore(g);
}

static void DrawScoreboardScreen(game_t *g)
{
    DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 160});
    Font f = g->hasFont ? g->font : GetFontDefault();

    if (!g->has_last_result) SubmitCurrentScore(g);

    // Pseudo field: mirrors pseudo_edit_buf into the real HTML <input>
    // every frame (web) so mobile's on-screen keyboard can edit it;
    // desktop draws/handles it directly since there's no browser input
    // to delegate to.
    Rectangle field = PseudoFieldRect();
#if defined(PLATFORM_WEB)
    pseudo_input_show(field.x, field.y, field.width, field.height, 22, g->pseudo_edit_buf);
#else
    DrawRectangleRounded(field, 0.2f, 6, (Color){35, 35, 42, 255});
    DrawRectangleRoundedLinesEx(field, 0.2f, 6, 2, (Color){130, 130, 145, 255});
    char display[SCOREBOARD_NAME_MAX + 2];
    snprintf(display, sizeof(display), "%s%s", g->pseudo_edit_buf,
             ((int)(GetTime() * 2) % 2 == 0) ? "|" : "");
    DrawTextEx(f, display, (Vector2){field.x + 10, field.y + 9}, 22, 1, RAYWHITE);
#endif
    bool committed = pseudo_input_update(g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
    if (committed) CommitPseudo(g);
    // Any edit (even before commit) should feel "live" per the request
    // -- once it no longer matches the last committed pseudo and isn't
    // empty, commit it as soon as focus would naturally be lost (Play
    // Again click, roll, etc. below); typing itself doesn't spam the
    // server on every keystroke, only on blur/Enter/those actions.

    // Roll button: circular arrow icon (drawn procedurally, two arcs +
    // arrowhead -- no extra asset needed).
    Rectangle roll = RollButtonRect();
    Vector2 rollCenter = { roll.x + roll.width / 2, roll.y + roll.height / 2 };
    Color rollColor = CheckCollisionPointRec(g->mpos, roll) ? (Color){110, 110, 125, 255} : (Color){75, 75, 88, 255};
    DrawCircleV(rollCenter, roll.width / 2, rollColor);
    float r = roll.width / 2 - 8;
    DrawRing(rollCenter, r - 2.5f, r, -40, 220, 24, RAYWHITE);
    // arrowhead at the ring's leading end (~220 degrees)
    float ang = 220.0f * DEG2RAD;
    Vector2 tip = { rollCenter.x + cosf(ang) * r, rollCenter.y + sinf(ang) * r };
    Vector2 perp = { -sinf(ang), cosf(ang) };
    Vector2 back = { rollCenter.x + cosf(ang) * (r - 6), rollCenter.y + sinf(ang) * (r - 6) };
    DrawTriangle(tip,
                 (Vector2){back.x + perp.x * 5, back.y + perp.y * 5},
                 (Vector2){back.x - perp.x * 5, back.y - perp.y * 5}, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(g->mpos, roll)) {
        scoreboard_random_pseudo(g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
        CommitPseudo(g);
    }

    char statsLine[64];
    if (g->has_last_result && g->last_result.ok) {
        snprintf(statsLine, sizeof(statsLine), "Best: %d   Tries: %d", g->last_result.best, g->last_result.tries);
    } else {
        snprintf(statsLine, sizeof(statsLine), "Score: %d (offline)", g->score->number);
    }
    Vector2 ss = MeasureTextEx(f, statsLine, 18, 1);
    DrawTextEx(f, statsLine, (Vector2){WIDTH / 2.0f - ss.x / 2, field.y + field.height + 8}, 18, 1, (Color){220, 220, 220, 255});

    // Live scoreboard list.
    float listW = 420;
    float listX = WIDTH / 2.0f - listW / 2;
    float rowY = field.y + field.height + 40;
    float rowH = 24;
    if (g->board_count == 0) {
        DrawTextEx(f, "No scores yet.", (Vector2){listX, rowY}, 18, 1, (Color){200, 200, 200, 255});
    }
    for (int i = 0; i < g->board_count && rowY + rowH < HEIGHT * 0.82f; i++) {
        char line[80];
        snprintf(line, sizeof(line), "%2d. %-20s %6d (%d tries)",
                 i + 1, g->board[i].name, g->board[i].best, g->board[i].tries);
        bool isMe = strcmp(g->board[i].name, g->pseudo) == 0;
        DrawTextEx(f, line, (Vector2){listX, rowY}, 18, 1, isMe ? (Color){255, 210, 90, 255} : RAYWHITE);
        rowY += rowH;
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
    g->final_wave_spawned = false;
    create_texts(g);
}

static void frame(void)
{
    float rawDt = GetFrameTime();
    get_delta_t(&g, rawDt);
    g.mpos = GetMousePosition();

    if (g.game_over) {
        // Play Again is only clickable outside the pseudo field's own
        // area, so a click meant for the text field never accidentally
        // restarts the run.
        Rectangle field = PseudoFieldRect();
        bool overField = CheckCollisionPointRec(g.mpos, field);
        if (!overField && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(g.mpos, PlayAgainButtonRect(&g))) {
#if defined(PLATFORM_WEB)
            pseudo_input_hide();
#endif
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
        DrawScoreboardScreen(&g);
        EndDrawing();
        return;
    }

#if defined(PLATFORM_WEB)
    pseudo_input_hide(); // not on the game-over screen -- keep the HTML field out of the way
#endif
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

    // Least-friction default pseudo (explicit user request): no "enter
    // your name" gate at all. If a pseudo was saved from a previous
    // visit, reuse it; otherwise roll a random LoL-themed one.
    if (!scoreboard_load_saved_pseudo(g.pseudo))
        scoreboard_random_pseudo(g.pseudo, SCOREBOARD_NAME_MAX);
    strncpy(g.pseudo_edit_buf, g.pseudo, SCOREBOARD_NAME_MAX);
    g.pseudo_edit_buf[SCOREBOARD_NAME_MAX] = '\0';

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
