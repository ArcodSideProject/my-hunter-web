/*
** scoreboard.c -- web implementation using synchronous XHR (simpler and
** more robust here than wiring emscripten_fetch's async callbacks or
** EM_ASYNC_JS through Asyncify for three tiny endpoints; the calls only
** ever happen on the game-over screen, so a brief synchronous block is
** an acceptable tradeoff and keeps the C side straightforward).
**
** Desktop build gets a local-only stub (see the #else branch) so native
** dev builds keep compiling/running without a server.
*/
#include "scoreboard.h"
#include <string.h>
#include <stdio.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

EM_JS(void, js_scoreboard_save_pseudo, (const char *name), {
    try {
        localStorage.setItem('my_hunter_pseudo', UTF8ToString(name));
    } catch (e) { /* localStorage unavailable (private mode, etc.) -- ignore */ }
});

// Returns a malloc'd (via stackAlloc-independent stringToNewUTF8) C
// string with the saved pseudo, or an empty string if none/unavailable.
// Caller must free() it.
EM_JS(char *, js_scoreboard_load_pseudo, (), {
    var v = '';
    try {
        v = localStorage.getItem('my_hunter_pseudo') || '';
    } catch (e) { /* ignore */ }
    var len = lengthBytesUTF8(v) + 1;
    var ptr = _malloc(len);
    stringToUTF8(v, ptr, len);
    return ptr;
});

// out must point to an int[3]: [0]=best, [1]=tries, [2]=ok(0/1).
EM_JS(void, js_scoreboard_submit, (const char *name, int score, const char *rename_from, int *out), {
    var result = [0, 0, 0];
    try {
        var body = { name: UTF8ToString(name), score: score };
        var renameFrom = UTF8ToString(rename_from);
        if (renameFrom) body.renameFrom = renameFrom;
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/api/score', false); // synchronous
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send(JSON.stringify(body));
        if (xhr.status >= 200 && xhr.status < 300) {
            var data = JSON.parse(xhr.responseText);
            result = [data.best | 0, data.tries | 0, 1];
        }
    } catch (e) { /* offline / server down -- result stays ok=0 */ }
    HEAP32[(out >> 2) + 0] = result[0];
    HEAP32[(out >> 2) + 1] = result[1];
    HEAP32[(out >> 2) + 2] = result[2];
});

// Returns a malloc'd JSON string of the full board (an array of
// {name,best,tries}), or an empty array "[]" on failure. Caller frees.
EM_JS(char *, js_scoreboard_fetch_all, (), {
    var text = '[]';
    try {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', '/api/scoreboard', false);
        xhr.send(null);
        if (xhr.status >= 200 && xhr.status < 300) text = xhr.responseText;
    } catch (e) { /* ignore, fall back to empty */ }
    var len = lengthBytesUTF8(text) + 1;
    var ptr = _malloc(len);
    stringToUTF8(text, ptr, len);
    return ptr;
});

bool scoreboard_load_saved_pseudo(char *out)
{
    char *raw = js_scoreboard_load_pseudo();
    strncpy(out, raw, SCOREBOARD_NAME_MAX);
    out[SCOREBOARD_NAME_MAX] = '\0';
    bool found = raw[0] != '\0';
    free(raw);
    return found;
}

void scoreboard_save_pseudo(const char *name)
{
    js_scoreboard_save_pseudo(name);
}

void scoreboard_submit(const char *name, int score, const char *rename_from, scoreboard_result_t *out)
{
    int raw[3] = {0, 0, 0};
    js_scoreboard_submit(name, score, rename_from ? rename_from : "", raw);
    out->best = raw[0];
    out->tries = raw[1];
    out->ok = raw[2] != 0;
}

// Extremely small hand-rolled JSON array-of-objects parser, just for
// this one shape: [{"name":"...","best":N,"tries":N}, ...]. Not a
// general JSON parser -- deliberately minimal since the only producer
// is our own server.js, whose output shape is fixed and trusted.
static const char *skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

int scoreboard_fetch_all(scoreboard_entry_t *out_entries)
{
    char *json = js_scoreboard_fetch_all();
    const char *p = json;
    int count = 0;

    p = skip_ws(p);
    if (*p != '[') { free(json); return 0; }
    p++;

    while (count < SCOREBOARD_MAX_ENTRIES) {
        p = skip_ws(p);
        if (*p == ']' || *p == '\0') break;
        if (*p == ',') { p++; continue; }
        if (*p != '{') break;
        p++;

        scoreboard_entry_t *e = &out_entries[count];
        memset(e, 0, sizeof(*e));

        while (*p != '}' && *p != '\0') {
            p = skip_ws(p);
            if (*p != '"') break;
            p++;
            const char *key_start = p;
            while (*p != '"' && *p != '\0') p++;
            int key_len = (int)(p - key_start);
            if (*p == '"') p++;
            p = skip_ws(p);
            if (*p == ':') p++;
            p = skip_ws(p);

            if (key_len == 4 && strncmp(key_start, "name", 4) == 0) {
                if (*p == '"') {
                    p++;
                    int i = 0;
                    while (*p != '"' && *p != '\0' && i < SCOREBOARD_NAME_MAX) {
                        e->name[i++] = *p++;
                    }
                    e->name[i] = '\0';
                    while (*p != '"' && *p != '\0') p++; // skip any overflow
                    if (*p == '"') p++;
                }
            } else if (key_len == 4 && strncmp(key_start, "best", 4) == 0) {
                e->best = (int)strtol(p, (char **)&p, 10);
            } else if (key_len == 5 && strncmp(key_start, "tries", 5) == 0) {
                e->tries = (int)strtol(p, (char **)&p, 10);
            } else {
                // unknown key -- skip its value crudely (number or string)
                if (*p == '"') {
                    p++;
                    while (*p != '"' && *p != '\0') p++;
                    if (*p == '"') p++;
                } else {
                    while (*p != ',' && *p != '}' && *p != '\0') p++;
                }
            }

            p = skip_ws(p);
            if (*p == ',') p++;
        }
        if (*p == '}') p++;
        count++;
    }

    free(json);
    return count;
}

#else /* !PLATFORM_WEB -- desktop stub, local-only, no server */

bool scoreboard_load_saved_pseudo(char *out)
{
    out[0] = '\0';
    return false;
}

void scoreboard_save_pseudo(const char *name)
{
    (void)name;
}

void scoreboard_submit(const char *name, int score, const char *rename_from, scoreboard_result_t *out)
{
    (void)name;
    (void)rename_from;
    out->best = score;
    out->tries = 1;
    out->ok = true;
}

int scoreboard_fetch_all(scoreboard_entry_t *out_entries)
{
    (void)out_entries;
    return 0;
}

#endif
