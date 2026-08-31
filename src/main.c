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
// pseudo row (editable text field + roll button) at the top, a Save
// button to actually commit to the server (local-first: nothing is
// sent until clicked), live local board below as a proper 3-column
// table (Name / Score / Tries), Play Again still available (drawn
// separately, partially behind the tree).
static Rectangle PseudoFieldRect(void)
{
    return (Rectangle){ WIDTH / 2.0f - 160, HEIGHT * 0.07f, 260, 40 };
}

static Rectangle RollButtonRect(void)
{
    Rectangle field = PseudoFieldRect();
    return (Rectangle){ field.x + field.width + 10, field.y, 40, 40 };
}

static Rectangle SaveButtonRect(void)
{
    Rectangle field = PseudoFieldRect();
    return (Rectangle){ field.x, field.y + field.height + 8, field.width + 50, 34 };
}

// Column x-offsets within the table, relative to its left edge.
#define COL_NAME_X 0
#define COL_SCORE_X 240
#define COL_TRIES_X 330
#define TABLE_WIDTH 420

static void SaveScore(game_t *g)
{
    // Local-first (explicit user request): only this explicit click
    // ever talks to the server. rename_from is the pseudo the LAST
    // successful Save used this run (not necessarily g->pseudo, which
    // may have been typed further since) -- NULL on the very first
    // Save of the run, so nothing gets renamed away on that one.
    const char *renameFrom = g->has_saved ? g->pseudo : NULL;
    if (g->pseudo_edit_buf[0] == '\0') return; // ignore an empty field
    if (renameFrom && strcmp(renameFrom, g->pseudo_edit_buf) == 0)
        renameFrom = NULL; // same name as last save -- not a rename, just another try
    strncpy(g->pseudo, g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
    g->pseudo[SCOREBOARD_NAME_MAX] = '\0';
    scoreboard_save_pseudo(g->pseudo);
    scoreboard_submit(g->pseudo, g->score->number, renameFrom, &g->last_result);
    g->has_saved = true;
    // Refresh the local board snapshot so the just-saved score is
    // reflected immediately.
    g->board_count = scoreboard_fetch_all(g->board);
    g->board_loaded = true;
}

static void DrawScoreboardScreen(game_t *g)
{
    DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 160});
    Font f = g->hasFont ? g->font : GetFontDefault();

    // Fetch the board once when the game-over screen first appears --
    // purely local/read-only after that until Save re-fetches it.
    if (!g->board_loaded) {
        g->board_count = scoreboard_fetch_all(g->board);
        g->board_loaded = true;
    }

    // Pseudo field: mirrors pseudo_edit_buf into the real HTML <input>
    // every frame (web) so mobile's on-screen keyboard can edit it;
    // desktop draws/handles it directly since there's no browser input
    // to delegate to.
    Rectangle field = PseudoFieldRect();
#if defined(PLATFORM_WEB)
    static bool pseudoFieldShownOnce = false;
    pseudo_input_show(field.x, field.y, field.width, field.height, 22, g->pseudo_edit_buf);
    if (!pseudoFieldShownOnce) {
        pseudo_input_set_value(g->pseudo_edit_buf); // seed the HTML field with the initial/default pseudo once
        pseudoFieldShownOnce = true;
    }
#else
    DrawRectangleRounded(field, 0.2f, 6, (Color){35, 35, 42, 255});
    DrawRectangleRoundedLinesEx(field, 0.2f, 6, 2, (Color){130, 130, 145, 255});
    char display[SCOREBOARD_NAME_MAX + 2];
    snprintf(display, sizeof(display), "%s%s", g->pseudo_edit_buf,
             ((int)(GetTime() * 2) % 2 == 0) ? "|" : "");
    DrawTextEx(f, display, (Vector2){field.x + 10, field.y + 9}, 22, 1, RAYWHITE);
#endif
    pseudo_input_update(g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
    // NOTE: purely local -- no auto-submit on Enter/blur anymore. The
    // Enter/blur "committed" signal from pseudo_input_update is
    // intentionally ignored here; Save (below) is now the only thing
    // that ever talks to the server, per explicit user request.

    // Roll button: circular-arrow "refresh" icon (drawn procedurally --
    // an open ring plus a triangular arrowhead tangent to it at the
    // ring's leading end, so it actually reads as a refresh/reroll
    // icon rather than an ambiguous blob).
    Rectangle roll = RollButtonRect();
    Vector2 rollCenter = { roll.x + roll.width / 2, roll.y + roll.height / 2 };
    Color rollColor = CheckCollisionPointRec(g->mpos, roll) ? (Color){110, 110, 125, 255} : (Color){75, 75, 88, 255};
    DrawCircleV(rollCenter, roll.width / 2, rollColor);
    float r = roll.width / 2 - 9;
    float startAngle = -60.0f, endAngle = 200.0f;
    DrawRing(rollCenter, r - 3.0f, r, startAngle, endAngle, 32, RAYWHITE);
    float endRad = endAngle * DEG2RAD;
    Vector2 ringTip = { rollCenter.x + cosf(endRad) * r, rollCenter.y + sinf(endRad) * r };
    Vector2 tangent = { -sinf(endRad), cosf(endRad) };
    Vector2 outward = { cosf(endRad), sinf(endRad) };
    float headLen = 11, headWidth = 8;
    Vector2 tip = { ringTip.x + tangent.x * headLen * 0.6f, ringTip.y + tangent.y * headLen * 0.6f };
    Vector2 base1 = {
        ringTip.x - tangent.x * headLen * 0.4f + outward.x * headWidth * 0.5f,
        ringTip.y - tangent.y * headLen * 0.4f + outward.y * headWidth * 0.5f
    };
    Vector2 base2 = {
        ringTip.x - tangent.x * headLen * 0.4f - outward.x * headWidth * 0.5f,
        ringTip.y - tangent.y * headLen * 0.4f - outward.y * headWidth * 0.5f
    };
    DrawTriangle(tip, base1, base2, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(g->mpos, roll)) {
        scoreboard_random_pseudo(g->pseudo_edit_buf, SCOREBOARD_NAME_MAX);
#if defined(PLATFORM_WEB)
        pseudo_input_set_value(g->pseudo_edit_buf); // push the roll into the HTML field explicitly
#endif
    }

    // Save button: the only thing that actually talks to the server.
    Rectangle save = SaveButtonRect();
    bool saveHover = CheckCollisionPointRec(g->mpos, save);
    Color saveColor = saveHover ? (Color){80, 160, 90, 255} : (Color){60, 130, 70, 255};
    DrawRectangleRounded(save, 0.25f, 8, saveColor);
    const char *saveLabel = g->has_saved ? "Save" : "Save score";
    Vector2 sls = MeasureTextEx(f, saveLabel, 18, 1);
    DrawTextEx(f, saveLabel, (Vector2){save.x + save.width / 2 - sls.x / 2, save.y + save.height / 2 - sls.y / 2}, 18, 1, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && saveHover) {
        SaveScore(g);
    }

    char statsLine[64];
    if (g->has_saved && g->last_result.ok) {
        snprintf(statsLine, sizeof(statsLine), "Saved -- Best: %d   Tries: %d", g->last_result.best, g->last_result.tries);
    } else if (g->has_saved) {
        snprintf(statsLine, sizeof(statsLine), "Save failed (offline?) -- Score: %d", g->score->number);
    } else {
        snprintf(statsLine, sizeof(statsLine), "Score: %d -- not saved yet", g->score->number);
    }
    Vector2 ss = MeasureTextEx(f, statsLine, 16, 1);
    DrawTextEx(f, statsLine, (Vector2){WIDTH / 2.0f - ss.x / 2, save.y + save.height + 8}, 16, 1, (Color){220, 220, 220, 255});

    // Live local scoreboard table (Name / Score / Tries columns).
    float tableX = WIDTH / 2.0f - TABLE_WIDTH / 2;
    float headerY = save.y + save.height + 34;
    float rowH = 22;
    Color headerColor = (Color){160, 160, 175, 255};
    DrawTextEx(f, "NAME", (Vector2){tableX + COL_NAME_X, headerY}, 15, 1, headerColor);
    DrawTextEx(f, "SCORE", (Vector2){tableX + COL_SCORE_X, headerY}, 15, 1, headerColor);
    DrawTextEx(f, "TRIES", (Vector2){tableX + COL_TRIES_X, headerY}, 15, 1, headerColor);
    DrawLineEx((Vector2){tableX, headerY + 20}, (Vector2){tableX + TABLE_WIDTH, headerY + 20}, 1, (Color){90, 90, 100, 255});

    float rowY = headerY + 28;
    if (g->board_count == 0) {
        DrawTextEx(f, "No scores yet.", (Vector2){tableX, rowY}, 16, 1, (Color){200, 200, 200, 255});
    }
    for (int i = 0; i < g->board_count && rowY + rowH < HEIGHT * 0.85f; i++) {
        bool isMe = g->has_saved && strcmp(g->board[i].name, g->pseudo) == 0;
        Color rowColor = isMe ? (Color){255, 210, 90, 255} : RAYWHITE;
        char nameCell[SCOREBOARD_NAME_MAX + 1];
        strncpy(nameCell, g->board[i].name, SCOREBOARD_NAME_MAX);
        nameCell[SCOREBOARD_NAME_MAX] = '\0';
        // Truncate long names so they don't run into the Score column.
        Vector2 nameSize = MeasureTextEx(f, nameCell, 16, 1);
        while (nameSize.x > COL_SCORE_X - 10 && strlen(nameCell) > 1) {
            nameCell[strlen(nameCell) - 1] = '\0';
            nameSize = MeasureTextEx(f, nameCell, 16, 1);
        }
        char scoreCell[16], triesCell[16];
        snprintf(scoreCell, sizeof(scoreCell), "%d", g->board[i].best);
        snprintf(triesCell, sizeof(triesCell), "%d", g->board[i].tries);
        DrawTextEx(f, nameCell, (Vector2){tableX + COL_NAME_X, rowY}, 16, 1, rowColor);
        DrawTextEx(f, scoreCell, (Vector2){tableX + COL_SCORE_X, rowY}, 16, 1, rowColor);
        DrawTextEx(f, triesCell, (Vector2){tableX + COL_TRIES_X, rowY}, 16, 1, rowColor);
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
    g->has_saved = false;
    g->board_loaded = false;
    g->final_wave_started = false;
    g->final_wave_done = false;
    g->final_wave_queue_count = 0;
    g->final_wave_queue_next = 0;
    g->final_wave_spawn_timer = 0;
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
