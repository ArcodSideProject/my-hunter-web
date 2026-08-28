/*
** EPITECH PROJECT, 2022 (raylib port)
** blur_tree -- ported 1:1. sfImage_getPixel is replaced by raylib's
** GetImageColor, same pixel-perfect alpha test on the CPU-side image
** copy (g->bg->tree_image).
*/
#include "my.h"

void blur_tree(game_t *g)
{
    frect_t rect = sprite_get_global_bounds(&g->bg->tree);
    int px = (int)((g->mpos.x - TREE_OFFSET_X * g->resize.x) / 3);
    int py = (int)((g->mpos.y - TREE_OFFSET_Y * g->resize.y) / 3);
    bool overTree = frect_contains(&rect, (int)g->mpos.x, (int)g->mpos.y) &&
        px >= 0 && py >= 0 && px < g->bg->tree_image.width && py < g->bg->tree_image.height &&
        GetImageColor(g->bg->tree_image, px, py).a != 0;

    if (overTree) {
        if (g->bg->tree_transparency > 150)
            g->bg->tree_transparency -= TREE_BLUR;
    } else {
        if (g->bg->tree_transparency < 255 - TREE_BLUR)
            g->bg->tree_transparency += TREE_BLUR;
        else {
            g->bg->tree_transparency = 255;
        }
    }
    sprite_set_color(&g->bg->tree, (Color){255, 255, 255,
                                            (unsigned char)g->bg->tree_transparency});
}
