/*
** EPITECH PROJECT, 2022 (raylib port)
** shadow -- ported 1:1
*/
#include "my.h"

void calculate_barrel_shadow(barrel_t *barrel)
{
    if (barrel->dead)
        return;
    float radius = barrel->rect.y > 5 * 20 ? barrel->rect.y / 20 : 5;
    shadow_set_radius(&barrel->shadow, radius);
    frect_t shadow_rect = shadow_get_global_bounds(&barrel->shadow);
    v2f pos = { barrel->rect.x + barrel->rect.width / 2 -
    shadow_rect.width / 2, HEIGHT - FLOOR_HEIGHT - 45 };
    shadow_set_position(&barrel->shadow, pos);
}
