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
    // pressing it once) progressively sends more and more barrels the
    // longer it's held -- starts slow (one spawn every 0.4s) and ramps
    // up to very fast (one every 0.015s, ~66/s) over 2.5s of continuous
    // hold, so the acceleration is clearly noticeable within a second
    // or two rather than a subtle change.
    if (!g->in_menu && !g->game_over) {
        if (IsKeyDown(KEY_ENTER)) {
            g->enter_hold_time += g->game_dt;
            g->enter_spawn_accum += g->game_dt;
            float t = g->enter_hold_time / 2.5f;
            if (t > 1.0f) t = 1.0f;
            // ease-in (t*t) so the ramp-up itself accelerates, making
            // the "progressively more and more" effect obvious
            float eased = t * t;
            float interval = 0.4f - eased * (0.4f - 0.015f);
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
