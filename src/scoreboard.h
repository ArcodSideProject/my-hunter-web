/*
** scoreboard.h -- global, server-backed high score list.
**
** Platform split:
**   - web (PLATFORM_WEB): backed by web/scoreboard.js via
**     EM_ASM/emscripten_fetch-free JS glue (simpler than wiring
**     emscripten_fetch's async callback dance for three tiny endpoints);
**     pseudo persisted in the browser's localStorage.
**   - desktop: local-only stub (no server, pseudo kept in-memory for the
**     session only) so native debug builds still compile and run without
**     needing a network round-trip -- this game only ships as a web
**     build, so the desktop path is purely for local dev/testing.
*/
#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <stdbool.h>

#define SCOREBOARD_NAME_MAX 20
#define SCOREBOARD_MAX_ENTRIES 100

typedef struct {
    char name[SCOREBOARD_NAME_MAX + 1];
    int best;
    int tries;
} scoreboard_entry_t;

typedef struct {
    int best;
    int tries;
    bool ok; // false if the request failed (offline, server error, ...)
} scoreboard_result_t;

// Loads the previously-saved pseudo (if any) into out (size
// SCOREBOARD_NAME_MAX+1). Returns true if a pseudo was found.
bool scoreboard_load_saved_pseudo(char *out);

// Persists the pseudo as the one to reuse next time.
void scoreboard_save_pseudo(const char *name);

// Submits one attempt's score for `name`, synchronously updating *out
// with the resulting best/tries (or ok=false on failure). Blocking on
// web via ASYNCIFY (already used elsewhere in this port for the loading
// screen), so callers don't need their own async state machine.
void scoreboard_submit(const char *name, int score, scoreboard_result_t *out);

// Fetches the full board (sorted by best desc), writing up to
// SCOREBOARD_MAX_ENTRIES into out_entries. Returns the number written,
// or 0 on failure.
int scoreboard_fetch_all(scoreboard_entry_t *out_entries);

#endif /* !SCOREBOARD_H */
