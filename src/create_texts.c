/*
** EPITECH PROJECT, 2022 (raylib port)
** create_texts -- ported 1:1 in spirit; string-building is done with
** snprintf/TextFormat at draw time instead of the original's manual
** my_strcat_int byte-twiddling, since raylib's text API doesn't need
** a persisted sfText object.
*/
#include "my.h"

void create_text(textint_t *t, char *string, v2f pos)
{
    t->string = malloc(sizeof(char) * 200);
    strcpy(t->string, string);
    t->number_index = (int)strlen(string);
    t->number = 0;
    t->pos = pos;
}

void create_texts(game_t *g)
{
    g->score = malloc(sizeof(textint_t));
    g->round = malloc(sizeof(textint_t));
    char *score_string = "Score: ";
    char *round_string = "Round: ";
    create_text(g->score, score_string, (v2f){20, 0});
    create_text(g->round, round_string, (v2f){WIDTH - 150, 0});
}
