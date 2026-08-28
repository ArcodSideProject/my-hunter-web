/*
** EPITECH PROJECT, 2022 (raylib port)
** create_background -- ported 1:1
*/
#include "my.h"

static void set_scale_to_all(bg_t *bg, v2f resize)
{
    sprite_set_scale(&bg->sky, resize);
    sprite_set_scale(&bg->clouds, resize);
    sprite_set_scale(&bg->mountain, resize);
    sprite_set_scale(&bg->far_woods, resize);
    sprite_set_scale(&bg->tiles, resize);
    sprite_set_scale(&bg->tree, resize);
    sprite_set_scale(&bg->front_grass, resize);
}

v2f create_background(bg_t *bg, textures_t *textures)
{
    bg->sky = create_sprite(textures->sky, (v2f){1, 1});
    bg->clouds = create_sprite(textures->clouds, (v2f){1, 1});
    bg->mountain = create_sprite(textures->mountain, (v2f){1, 1});
    bg->far_woods = create_sprite(textures->far_woods, (v2f){1, 1});
    bg->tiles = create_sprite(textures->tiles, (v2f){1, 1});
    bg->tree = create_sprite(textures->tree, (v2f){1, 1});
    bg->front_grass = create_sprite(textures->front_grass, (v2f){1, 1});
    v2f resize = { WIDTH / sprite_get_global_bounds(&bg->sky).width,
    HEIGHT / sprite_get_global_bounds(&bg->sky).height };
    set_scale_to_all(bg, resize);

    bg->clouds_speed = 0;
    bg->tree_transparency = 255;
    sprite_set_position(&bg->tree, (v2f){TREE_OFFSET_X * resize.x,
    TREE_OFFSET_Y * resize.y});
    bg->tree_image = LoadImage("assets/background/Tree.png");
    return resize;
}
