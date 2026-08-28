/*
** EPITECH PROJECT, 2022 (raylib port)
** update_texts -- in the original this rebuilt the cached sfText string
** every frame from t->number. In this port draw_text_int (draw.c)
** builds the display string directly from t->number at draw time via
** snprintf, so there is no separate text object to keep in sync --
** this function is kept as a structural no-op call site so main.c's
** loop still reads like the original's render_window().
*/
#include "my.h"

void update_texts(game_t *g)
{
    (void)g;
}
