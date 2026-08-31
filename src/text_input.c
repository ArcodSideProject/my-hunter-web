/*
** text_input.c -- text field handling for raylib.
** Not part of the original game; added for the scoreboard pseudo entry.
**
** Two backends:
**   - desktop: printable-ASCII GetCharPressed() polling, works fine
**     with a physical keyboard.
**   - web (PLATFORM_WEB): raylib's canvas can't trigger a mobile
**     browser's on-screen keyboard on its own, so editing is delegated
**     to a real HTML <input> overlaid on the canvas (see
**     web/shell.html's pseudoInputShow/Hide/GetValue/ConsumeCommitted).
*/
#include "my.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

EM_JS(void, js_pseudo_input_show, (float x, float y, float w, float h, float fontSize), {
    if (typeof window.pseudoInputShow === 'function') {
        window.pseudoInputShow(x, y, w, h, fontSize);
    }
});

EM_JS(void, js_pseudo_input_set_value, (const char *value), {
    if (typeof window.pseudoInputSetValue === 'function') {
        window.pseudoInputSetValue(UTF8ToString(value));
    }
});

EM_JS(void, js_pseudo_input_hide, (), {
    if (typeof window.pseudoInputHide === 'function') window.pseudoInputHide();
});

EM_JS(char *, js_pseudo_input_get_value, (), {
    var v = (typeof window.pseudoInputGetValue === 'function') ? window.pseudoInputGetValue() : '';
    var len = lengthBytesUTF8(v) + 1;
    var ptr = _malloc(len);
    stringToUTF8(v, ptr, len);
    return ptr;
});

EM_JS(int, js_pseudo_input_consume_committed, (), {
    return (typeof window.pseudoInputConsumeCommitted === 'function' &&
            window.pseudoInputConsumeCommitted()) ? 1 : 0;
});

void pseudo_input_show(float x, float y, float w, float h, float font_size, const char *value)
{
    (void)value; // no longer force-pushed every frame -- see set_value below
    js_pseudo_input_show(x, y, w, h, font_size);
}

void pseudo_input_set_value(const char *value)
{
    js_pseudo_input_set_value(value);
}

void pseudo_input_hide(void)
{
    js_pseudo_input_hide();
}

bool pseudo_input_update(char *buf, int max_len)
{
    char *val = js_pseudo_input_get_value();
    strncpy(buf, val, max_len);
    buf[max_len] = '\0';
    free(val);
    return js_pseudo_input_consume_committed() != 0;
}

#else /* !PLATFORM_WEB -- desktop: draw the field ourselves, no HTML overlay needed */

void pseudo_input_show(float x, float y, float w, float h, float font_size, const char *value)
{
    (void)x; (void)y; (void)w; (void)h; (void)font_size; (void)value;
}

void pseudo_input_set_value(const char *value)
{
    (void)value; // desktop reads/writes the buffer directly in main.c, nothing to sync
}

void pseudo_input_hide(void)
{
}

bool pseudo_input_update(char *buf, int max_len)
{
    int c = GetCharPressed();
    while (c > 0) {
        int len = (int)strlen(buf);
        if (c >= 32 && c < 127 && len < max_len) {
            buf[len] = (char)c;
            buf[len + 1] = '\0';
        }
        c = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(buf);
        if (len > 0) buf[len - 1] = '\0';
    }
    return IsKeyPressed(KEY_ENTER);
}

#endif

// League of Legends-flavored default pseudo generator (explicit user
// request): either a champion name, an item name, or a possessive
// combo of the two ("Annie's Bear", "Anivia's Egg", "Nashor's Tooth").
// Small hand-picked lists, not exhaustive -- just enough variety.
static const char *CHAMPIONS[] = {
    "Annie", "Anivia", "Ashe", "Blitzcrank", "Darius", "Ezreal", "Garen",
    "Jinx", "Katarina", "LeeSin", "Lulu", "Nasus", "Rammus", "Riven",
    "Shaco", "Teemo", "Thresh", "Tristana", "Veigar", "Yasuo", "Yorick",
    "Ziggs", "Zoe",
};
static const char *ITEMS[] = {
    "Nashor's Tooth", "Infinity Edge", "Doran's Blade", "Sunfire Aegis",
    "Rabadon's Deathcap", "Trinity Force", "Zhonya's Hourglass",
    "Guardian Angel", "Black Cleaver", "Bloodthirster",
};
// Only entries whose champion actually has this exact item/pet in lore
// pair up here -- kept small and deliberately funny, not random nouns.
typedef struct { const char *champ; const char *thing; } combo_t;
static const combo_t COMBOS[] = {
    {"Annie", "Bear"}, {"Anivia", "Egg"}, {"Teemo", "Mushroom"},
    {"Heimerdinger", "Turret"}, {"Malzahar", "Voidling"},
    {"Yorick", "Ghoul"}, {"Shyvana", "Egg"}, {"Zilean", "Bomb"},
};

void scoreboard_random_pseudo(char *out, int max_len)
{
    int roll = rand() % 3;
    if (roll == 0) {
        const char *c = CHAMPIONS[rand() % (int)(sizeof(CHAMPIONS) / sizeof(CHAMPIONS[0]))];
        strncpy(out, c, max_len);
    } else if (roll == 1) {
        const char *it = ITEMS[rand() % (int)(sizeof(ITEMS) / sizeof(ITEMS[0]))];
        strncpy(out, it, max_len);
    } else {
        combo_t c = COMBOS[rand() % (int)(sizeof(COMBOS) / sizeof(COMBOS[0]))];
        snprintf(out, max_len + 1, "%s's %s", c.champ, c.thing);
    }
    out[max_len] = '\0';
}
