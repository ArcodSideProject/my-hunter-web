/*
** EPITECH PROJECT, 2022 (raylib port)
** event_handler -- adapted from SFML's poll-event queue model to
** raylib's per-frame input-state polling. manage_mouse_click/
** manage_keys keep their original names and logic; this function just
** detects the "pressed this frame" transitions raylib exposes directly
** (IsMouseButtonPressed/IsKeyPressed), equivalent to SFML's
** sfEvtMouseButtonPressed/sfEvtKeyPressed events.
*/
#include "my.h"

static int raylib_key_to_original(int key)
{
    if (key == KEY_ENTER) return 1; // sfKeyEnter marker
    if (key == KEY_SPACE) return 2; // sfKeySpace marker
    return 0;
}

void event_handler(game_t *g)
{
    if (WindowShouldClose())
        g->game_over = true;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        manage_mouse_click(g);
    int key = GetKeyPressed();
    while (key != 0) {
        int mapped = raylib_key_to_original(key);
        if (mapped != 0)
            manage_keys(g, mapped);
        key = GetKeyPressed();
    }

    // Enhancement (explicit user request): holding ENTER (not just
    // pressing it once) ramps barrel spawn rate up the longer it's
    // held -- starts at one spawn/0.35s, ramps to one/0.02s over 3s.
    if (!g->in_menu && !g->game_over) {
        if (IsKeyDown(KEY_ENTER)) {
            g->enter_hold_time += g->game_dt;
            g->enter_spawn_accum += g->game_dt;
            float t = g->enter_hold_time / 3.0f;
            if (t > 1.0f) t = 1.0f;
            float interval = 0.35f - t * (0.35f - 0.02f);
            while (g->enter_spawn_accum > interval) {
                spawn_barrel(g, 1);
                g->enter_spawn_accum -= interval;
            }
        } else {
            g->enter_hold_time = 0;
            g->enter_spawn_accum = 0;
        }
    }
}
