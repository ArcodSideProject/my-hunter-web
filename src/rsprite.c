/*
** rsprite_t / rshape_t implementation -- raylib-backed stand-ins for the
** CSFML sfSprite / sfCircleShape calls the original code used everywhere.
** Function names mirror the sf* calls 1:1 in spirit (create_sprite,
** sprite_set_position, sprite_move, sprite_get_global_bounds, etc.) so
** the ported logic files read as close to the original as possible.
*/
#include "my.h"

rsprite_t create_sprite(Texture2D texture, v2f scale)
{
    rsprite_t sp;
    sp.texture = texture;
    sp.hasTexture = (texture.id != 0);
    sp.position = (v2f){0, 0};
    sp.scale = scale;
    sp.rotation = 0;
    sp.origin = (v2f){0, 0};
    sp.color = WHITE;
    sp.textureRect = (Rectangle){0, 0, (float)texture.width, (float)texture.height};
    return sp;
}

// mirrors sfSprite_getGlobalBounds: axis-aligned bounding box of the
// sprite AFTER scale is applied (rotation is ignored for AABB purposes,
// same simplification SFML itself makes available via getGlobalBounds
// on an unrotated/rotated sprite -- the original never rotates sprites
// it later measures with getGlobalBounds, only barrels, whose rotation
// is purely cosmetic and never re-queried via rect, so this holds).
frect_t sprite_get_global_bounds(rsprite_t *sp)
{
    float w = sp->textureRect.width * fabsf(sp->scale.x);
    float h = sp->textureRect.height * fabsf(sp->scale.y);
    return (Rectangle){
        sp->position.x - sp->origin.x * sp->scale.x,
        sp->position.y - sp->origin.y * sp->scale.y,
        w, h
    };
}

void sprite_set_position(rsprite_t *sp, v2f pos) { sp->position = pos; }

void sprite_move(rsprite_t *sp, v2f delta)
{
    sp->position.x += delta.x;
    sp->position.y += delta.y;
}

void sprite_set_texture_rect(rsprite_t *sp, int_rect rect) { sp->textureRect = rect; }
void sprite_set_color(rsprite_t *sp, Color c) { sp->color = c; }
void sprite_rotate(rsprite_t *sp, float degrees) { sp->rotation += degrees; }
void sprite_set_origin(rsprite_t *sp, v2f origin) { sp->origin = origin; }
void sprite_set_scale(rsprite_t *sp, v2f scale) { sp->scale = scale; }
void sprite_set_texture(rsprite_t *sp, Texture2D texture)
{
    sp->texture = texture;
    sp->hasTexture = (texture.id != 0);
    sp->textureRect = (Rectangle){0, 0, (float)texture.width, (float)texture.height};
}

void draw_sprite(rsprite_t *sp)
{
    if (!sp->hasTexture) return;
    Rectangle src = sp->textureRect;
    Rectangle dst = {
        sp->position.x, sp->position.y,
        src.width * sp->scale.x, src.height * sp->scale.y
    };
    Vector2 origin = { sp->origin.x * sp->scale.x, sp->origin.y * sp->scale.y };
    DrawTexturePro(sp->texture, src, dst, origin, sp->rotation, sp->color);
}

rshape_t create_shadow(v2f scale)
{
    rshape_t shadow;
    shadow.radius = 40;
    shadow.position = (v2f){0, 0};
    shadow.scale = scale;
    shadow.fillColor = (Color){0, 0, 0, 150};
    return shadow;
}

void shadow_set_radius(rshape_t *shadow, float radius) { shadow->radius = radius; }
void shadow_set_position(rshape_t *shadow, v2f pos) { shadow->position = pos; }

frect_t shadow_get_global_bounds(rshape_t *shadow)
{
    float w = shadow->radius * 2.0f * shadow->scale.x;
    float h = shadow->radius * 2.0f * shadow->scale.y;
    return (Rectangle){shadow->position.x, shadow->position.y, w, h};
}

void draw_shadow(rshape_t *shadow)
{
    float rx = shadow->radius * shadow->scale.x;
    float ry = shadow->radius * shadow->scale.y;
    Vector2 center = { shadow->position.x + rx, shadow->position.y + ry };
    DrawEllipse((int)center.x, (int)center.y, rx, ry, shadow->fillColor);
}

bool frect_contains(frect_t *r, int x, int y)
{
    return x >= r->x && x <= r->x + r->width && y >= r->y && y <= r->y + r->height;
}
