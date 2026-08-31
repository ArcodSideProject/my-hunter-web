/*
** raylib port of MyHunter -- my.h
** Faithful adaptation of the original CSFML my.h: same struct layout,
** same constants, same function names wherever possible. CSFML types
** (sfSprite*, sfClock*, sfCircleShape*, sfFloatRect, sfIntRect) are
** replaced by small raylib-based equivalents defined below so the
** rest of the ported .c files can stay as close as possible to the
** original logic.
*/
#ifndef MY_H
#define MY_H

#include "raylib.h"
#include "scoreboard.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

///////////////
// CONSTANTS //
///////////////
#define FPS 60
#define GAME_TICK 20

#define WIDTH 1152
#define HEIGHT 672

#define GRAVITY 5
#define AIR_FRICTION 0.98f
#define FLOOR_FRICTION 0.40f
#define GRAGAS_FRICTION 0.80f

#define FLOOR_HEIGHT 55
#define G_FLOOR_HEIGHT 80

#define TREE_OFFSET_X 173
#define TREE_OFFSET_Y 82
#define TREE_BLUR 8

#define EXPLOSION_WIDTH 50
#define EXPLOSION_HEIGHT 51
#define SQUATTING_GRAGAS_WIDTH 50
#define SQUATTING_GRAGAS_HEIGHT 44
#define STANDING_GRAGAS_WIDTH 66
#define STANDING_GRAGAS_HEIGHT 62
#define STANDING_GRAGAS_HEIGHT_OFFSET 46

////////////
// MACROS //
////////////
#define ABS(x) ((x) < 0 ? (x) * -1 : (x))

/////////////
// TYPEDEF //
/////////////
typedef Vector2 v2f;
typedef Rectangle frect_t; // stands in for sfFloatRect (left=x,top=y,width,height)
typedef Rectangle int_rect; // stands in for sfIntRect, used as a texture-space rect

// ---- rsprite_t: minimal stand-in for sfSprite*, carrying exactly the
// fields the original code actually calls setters/getters on. This is
// the only real structural addition versus the original -- everywhere
// the original called sfSprite_setPosition/move/getGlobalBounds/etc.,
// this file's helper functions below (mirroring those exact names)
// operate on this struct instead.
typedef struct {
    Texture2D texture;
    bool hasTexture;
    v2f position;
    v2f scale;
    float rotation;   // degrees
    v2f origin;        // in *unscaled* texture pixels, matches sfSprite_setOrigin semantics
    Color color;
    int_rect textureRect; // source rect within the texture (0,0,tex.width,tex.height) by default
} rsprite_t;

// ---- rshape_t: minimal stand-in for sfCircleShape* (used only for the
// barrel/Gragas ground shadows in the original).
typedef struct {
    float radius;
    v2f position;   // top-left of bounding box, matches sfCircleShape_setPosition
    v2f scale;
    Color fillColor;
} rshape_t;

////////////////
// STRUCTURES //
////////////////

typedef struct text_and_int {
    char *string;
    int number_index;
    int number;
    v2f pos;
} textint_t;

typedef struct barrel {
    rsprite_t sp;
    frect_t rect;
    v2f scale;
    int max_health;
    int health;
    bool dead;
    bool at_floor;
    int explosion_state;
    Texture2D explosion_texture;
    bool spawning;
    int_rect rect_anim;
    float clock; // seconds elapsed since last restart, mirrors sfClock*
    v2f acceleration;
    v2f velocity;
    float angle;
    rshape_t shadow;
    // Scoreboard "final wave" barrel (see scoreboard.c): empty string
    // ("") for every normal barrel -- no label drawn. Fixed-size (no
    // malloc/free) since it's always either empty or one scoreboard
    // name, capped at SCOREBOARD_NAME_MAX.
    char scoreboard_name[SCOREBOARD_NAME_MAX + 1];
    // Whether killing this barrel awards g->score (explicit user
    // request: barrels spawned by holding ENTER shouldn't count).
    // Every normal spawn path sets this true; only manage_keys()'s
    // ENTER handler sets it false.
    bool gives_score;
    struct barrel *next_barrel;
} barrel_t;

typedef struct gragas {
    rsprite_t sp;
    frect_t rect;
    int_rect rect_anim;
    bool spawning;
    int spawn_animation;
    v2f acceleration;
    v2f velocity;
    float spawn_anim_clock;
    bool has_spawn_anim_clock;
    float jump_anim_clock;
    float clock;
    bool jumping;
    bool at_floor;
    textint_t *gscore;
    rshape_t shadow;
} gragas_t;

typedef struct bg {
    rsprite_t sky;
    rsprite_t clouds;
    float clouds_speed;
    rsprite_t mountain;
    rsprite_t far_woods;
    rsprite_t tiles;
    rsprite_t tree;
    Image tree_image; // CPU-side copy, for blur_tree's pixel-perfect alpha test
    unsigned short tree_transparency;
    rsprite_t front_grass;
} bg_t;

typedef struct textures {
    Texture2D start_button;
    Texture2D barrel;
    Texture2D explosion;
    Texture2D sight;
    Texture2D gragas;
    Texture2D sky;
    Texture2D clouds;
    Texture2D mountain;
    Texture2D far_woods;
    Texture2D tiles;
    Texture2D tree;
    Texture2D front_grass;
} textures_t;

typedef struct game {
    textures_t *textures;
    v2f resize;
    rsprite_t start_button;
    bg_t *bg;
    barrel_t *barrel;
    gragas_t *gragas;
    rsprite_t sight;
    Vector2 mpos;
    Font font;
    bool hasFont;
    textint_t *score;
    textint_t *round;
    float round_clock;
    float game_dt; // this frame's raw (unscaled) delta, from raylib's GetFrameTime()
    float dt;       // GAME_TICK-scaled dt, matches the original's g->dt
    int barrel_count;
    int barrels_spawned;
    bool in_menu;
    bool game_over;

    // Enhancement (explicit user request, not in the original): holding
    // ENTER ramps up barrel spawn rate the longer it's held, instead of
    // the original's single-spawn-per-press only.
    float enter_hold_time;
    float enter_spawn_accum;

    // Scoreboard (explicit user request, not in the original): a global,
    // server-backed high score list keyed by a player-chosen pseudo.
    // Local-first model (explicit user request, replacing an earlier
    // auto-submit-on-every-edit design that was buggy/surprising):
    // the pseudo field and board are edited/browsed purely locally;
    // nothing reaches the server until the player explicitly clicks
    // Save. See scoreboard.c/scoreboard.h and web/shell.html's
    // pseudo-input glue (mobile needs a real HTML <input> to get an
    // on-screen keyboard; raylib's canvas can't trigger one itself).
    char pseudo[SCOREBOARD_NAME_MAX + 1];       // last name actually committed to the server via Save
    char pseudo_edit_buf[SCOREBOARD_NAME_MAX + 1]; // live text field contents (may differ from pseudo until Save)
    bool has_saved;            // true once Save has been clicked at least once this run
    scoreboard_result_t last_result; // best/tries returned by the last successful Save
    scoreboard_entry_t board[SCOREBOARD_MAX_ENTRIES]; // local cache of the full list, fetched once on game over
    int board_count;
    bool board_loaded;

    // Final wave (explicit user request): scoreboard-entry barrels are
    // trickled in one at a time at a high rate during round 5's last
    // wave, not spawned all at once. See spawn_scoreboard_final_wave.c.
    bool final_wave_started; // queue fetched, trickle spawn in progress
    bool final_wave_done;    // every queued entry has been spawned (may still be alive)
    scoreboard_entry_t final_wave_queue[SCOREBOARD_MAX_ENTRIES];
    int final_wave_queue_count;
    int final_wave_queue_next; // index of the next entry still to spawn
    float final_wave_spawn_timer;
} game_t;

///////////////////////////////////////////////
// rsprite_t / rshape_t helpers (sfSprite-like API)
///////////////////////////////////////////////
rsprite_t create_sprite(Texture2D texture, v2f scale);
frect_t sprite_get_global_bounds(rsprite_t *sp);
void sprite_set_position(rsprite_t *sp, v2f pos);
void sprite_move(rsprite_t *sp, v2f delta);
void sprite_set_texture_rect(rsprite_t *sp, int_rect rect);
void sprite_set_color(rsprite_t *sp, Color c);
void sprite_rotate(rsprite_t *sp, float degrees);
void sprite_set_origin(rsprite_t *sp, v2f origin);
void sprite_set_scale(rsprite_t *sp, v2f scale);
void sprite_set_texture(rsprite_t *sp, Texture2D texture);
void draw_sprite(rsprite_t *sp);

rshape_t create_shadow(v2f scale);
void shadow_set_radius(rshape_t *shadow, float radius);
void shadow_set_position(rshape_t *shadow, v2f pos);
frect_t shadow_get_global_bounds(rshape_t *shadow);
void draw_shadow(rshape_t *shadow);

bool frect_contains(frect_t *r, int x, int y);

///////////////////////
// FUNCTIONS INCLUDE //
///////////////////////

void get_delta_t(game_t *g, float rawDt);
void event_handler(game_t *g);
void manage_mouse_click(game_t *g);
void manage_keys(game_t *g, int key_code);
void move_sight_to_cursor(game_t *g);
void blur_tree(game_t *g);
void pseudo_input_show(float x, float y, float w, float h, float font_size, const char *value);
void pseudo_input_set_value(const char *value);
void pseudo_input_hide(void);
bool pseudo_input_update(char *buf, int max_len);
void scoreboard_random_pseudo(char *out, int max_len);

void animate_gragas(gragas_t *gragas, float dt, float rawDt, barrel_t *barrel);
void animate_gragas_spawn(gragas_t *gragas, float rawDt);
void animate_barrels(int barrel_count, barrel_t *barrel, float dt, float rawDt);
void goto_barrel(gragas_t *gragas, barrel_t *barrel, float rawDt);
void gragas_touch(gragas_t *gragas, Vector2 mpos);

void move_clouds(rsprite_t *clouds, float *clouds_speed, int score);

void draw_on_screen(game_t *g);
void draw_on_screen_background_and_gameplay(game_t *g);
void draw_on_screen_foreground(game_t *g);

void spawn_round(game_t *g, float rawDt);

void initialize_game(game_t *g);
void create_barrel(barrel_t *barrel, int max_health, Texture2D barrel_texture,
Texture2D explosion);
v2f create_background(bg_t *bg, textures_t *textures);
void create_texts(game_t *g);
void create_text(textint_t *t, char *string, v2f pos);
void create_gragas(gragas_t *gragas, Texture2D gragas_texture);
rsprite_t create_start_button(Texture2D start_button_texture);

void spawn_barrel(game_t *g, int max_health);
void spawn_scoreboard_final_wave_tick(game_t *g, float rawDt);
void bounce_on_border(barrel_t *barrel, float rawDt);
void for_touched_barrels(barrel_t *barrel, Vector2 mpos);
void kill_barrel(barrel_t *barrel);
void remove_dead_barrels(game_t *g);
void calculate_barrel_shadow(barrel_t *barrel);

void free_background(bg_t *bg);
void free_barrel(barrel_t *barrel);
void free_barrels(barrel_t *barrel, int barrel_count);
void draw_barrels(barrel_t *barrel, bool spawning, int barrel_count, Font font, bool hasFont);
void free_gragas(gragas_t *gragas);
void big_free(game_t *g);

void update_texts(game_t *g);

v2f int_multiply_v2f(v2f v, float x);
v2f add_two_v2f(v2f v1, v2f v2);

int rd_1(float *spawn_rate, int *barrel_health, int barrels_spawned);
int rd_2(float *spawn_rate, int *barrel_health, int barrels_spawned);
int rd_3(float *spawn_rate, int *barrel_health, int barrels_spawned);
int rd_4(float *spawn_rate, int *barrel_health, int barrels_spawned);
int rd_5(float *spawn_rate, int *barrel_health, game_t *g, float rawDt);

#endif /* !MY_H */
