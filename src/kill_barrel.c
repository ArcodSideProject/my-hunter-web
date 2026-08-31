/*
** EPITECH PROJECT, 2022 (raylib port)
** kill_barrel -- ported 1:1
*/
#include "my.h"

void kill_barrel(barrel_t *barrel)
{
    // Refresh rect from the sprite's ACTUAL current position first --
    // barrel->rect is only updated once per frame at the top of
    // animate_barrel() in barrels.c, BEFORE that same frame's movement
    // is applied. By the time kill_barrel() runs later in the frame
    // (via goto_barrel.c's touching_barrel, or here via
    // remove_dead_barrels on a barrel that died from a player click
    // the previous frame), barrel->rect can be one full frame of
    // movement stale -- at the velocities barrels reach right after a
    // click impulse (velocity.y=-60 etc), that's tens of pixels of
    // real, visible gap between the barrel's actual position and
    // where the explosion anchors. This is the real cause of barrels
    // "exploding somewhere else, like physics were still being
    // calculated". Refreshing here ensures the explosion always
    // anchors to wherever the sprite genuinely is right now.
    barrel->rect = sprite_get_global_bounds(&barrel->sp);

    // Anchor the explosion visually at the barrel's real CENTER point
    // before switching textures/scale/origin -- the origin set in
    // adapt_origin_to_rotate() was computed against the BARREL texture's
    // own dimensions/scale (barrel is 22x28 native * 3x scale), which
    // becomes meaningless once the sprite switches to the much larger
    // explosion sheet (50x51 native) at a different scale (1.9x); left
    // as-is, that stale origin offset the explosion's apparent position
    // by tens of pixels from where the barrel actually died.
    float centerX = barrel->rect.x + barrel->rect.width / 2.0f;
    float centerY = barrel->rect.y + barrel->rect.height / 2.0f;
    barrel->health = 1;
    barrel->dead = true;
    sprite_set_texture(&barrel->sp, barrel->explosion_texture);
    sprite_set_color(&barrel->sp, WHITE);
    sprite_set_scale(&barrel->sp, (v2f){1.9f, 1.9f});
    sprite_set_origin(&barrel->sp, (v2f){EXPLOSION_WIDTH / 2.0f, EXPLOSION_HEIGHT / 2.0f});
    sprite_set_position(&barrel->sp, (v2f){centerX, centerY});
    barrel->clock = 0;
    // shadow has no heap allocation in this port (plain struct field),
    // nothing to destroy -- kept as a no-op call site for structural
    // parity with the original's sfCircleShape_destroy(barrel->shadow).
}

void free_barrel(barrel_t *barrel)
{
    free(barrel);
}

static void remove_in_list(barrel_t *barrel, int *barrel_count, int *score_nb)
{
    barrel_t *save;
    do {
        if (barrel->next_barrel->health <= 0) {
            bool givesScore = barrel->next_barrel->gives_score;
            kill_barrel(barrel->next_barrel);
            if (givesScore)
                (*score_nb)++;
        } if (barrel->next_barrel->explosion_state > 5) {
            (*barrel_count)--;
            save = barrel->next_barrel->next_barrel;
            free_barrel(barrel->next_barrel);
            barrel->next_barrel = save;
        }
        if (barrel->next_barrel == NULL)
            return;
        barrel = barrel->next_barrel;
    } while (barrel->next_barrel != NULL);
}

void remove_dead_barrels(game_t *g)
{
    if (g->barrel_count <= 0)
        return;
    barrel_t *save;
    while (g->barrel != NULL && (g->barrel->health <= 0 ||
    g->barrel->explosion_state > 5)) {
        if (g->barrel->health <= 0) {
            bool givesScore = g->barrel->gives_score;
            kill_barrel(g->barrel);
            if (givesScore)
                g->score->number++;
        } if (g->barrel->explosion_state > 5) {
            save = g->barrel->next_barrel;
            g->barrel_count--;
            free_barrel(g->barrel);
            g->barrel = save;
        }
    }
    if (g->barrel_count > 1)
        remove_in_list(g->barrel, &g->barrel_count, &g->score->number);
}
