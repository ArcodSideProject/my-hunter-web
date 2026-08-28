# My Hunter — raylib port

A **direct port** of Epitech's tek1 "MyHunter"
(`EpitechPromo2027/B-MUL-100-LYN-1-1-myhunter-antoine.esman`), keeping
the original C game logic essentially untouched and swapping only the
CSFML graphics/window/input calls for [raylib](https://www.raylib.com)
equivalents. Targets native desktop **and** the web (via Emscripten/WASM).

## What "direct port" actually means here

Every `.c` file under `src/` corresponds 1:1 to a file in the original
repo, keeping the same function names, same struct layout (including
the original's singly-linked list of barrels — not converted to an
array), and the same physics/gameplay constants and formulas verbatim:

- `bounce.c`, `barrels.c`, `barrel_touch.c`, `kill_barrel.c`, `shadow.c`,
  `spawn_barrel.c` — barrel physics/lifecycle, unchanged math
- `gragas.c`, `goto_barrel.c`, `gragas_touch.c`, `animate_spawn.c` —
  Gragas's AI/movement/animation, unchanged math
- `rounds.c`, `easy_rounds.c` — the 5-round difficulty table, unchanged
- `blur_tree.c`, `clouds.c`, `draw.c` — background effects, unchanged
- `create_*.c`, `initialize_game.c`, `free.c` — setup/teardown
- `event_handler.c`, `keys.c`, `mouse.c` — input handling
- `main.c` — the original's `render_window()` loop, same call order

The only real addition is `rsprite.c` / `my.h`'s `rsprite_t`/`rshape_t`
structs: small stand-ins for `sfSprite*`/`sfCircleShape*` that expose
the same function names the original called everywhere
(`sprite_set_position`, `sprite_move`, `sprite_get_global_bounds`,
`draw_sprite`, `shadow_set_radius`, ...) but are backed by a raylib
`Texture2D` + position/scale/rotation/origin/color fields instead of an
SFML object. Every call site in the ported logic files reads almost
identically to the CSFML original.

**Adaptations required by the platform switch** (not gameplay changes):
- SFML's clock objects (`sfClock*`) become plain `float` seconds
  counters, advanced by raylib's `GetFrameTime()` each frame instead of
  being read via `sfClock_getElapsedTime()`
- SFML's poll-event queue (`sfRenderWindow_pollEvent`) becomes raylib's
  per-frame input-state polling (`IsMouseButtonPressed`/`GetKeyPressed`)
  in `event_handler.c` — same `manage_mouse_click`/`manage_keys`
  functions get called, just triggered differently
- Window creation/destruction moves to `main.c` (raylib's
  `InitWindow`/`CloseWindow` instead of `sfRenderWindow_create`), since
  raylib needs its window to exist before textures can load
- Text rendering (`create_text`/`update_texts`) drops the original's
  manual string-building (`my_strcat_int` etc.) in favor of `snprintf`
  at draw time, since raylib doesn't need a persisted text object the
  way `sfText` did
- `sfRenderWindow_close()` (used by round 5's win condition) becomes a
  `g->game_over` flag checked by the main loop, since raylib doesn't
  have an equivalent "close the window from game logic" call

**One explicitly-requested enhancement, layered on top, not replacing
anything**: holding ENTER (not just pressing it once) ramps up barrel
spawn rate the longer it's held — starts at one spawn every 0.35s, down
to one every 0.02s over 3 seconds of continuous hold. The original's
single-press-spawns-one-barrel behavior on ENTER is untouched; this is
purely additive (see `event_handler.c`).

## Real assets

All original sprite sheets, backgrounds, and the font are included
under `assets/` (`barrel.png`, `sumos.png`, `explosion.png`,
`start_button.png`, all 7 background layers, `upheavtt.ttf`, the red
crosshair sight) — pulled from the real repo, not recreated.

## Build

### Native (desktop)
```sh
mkdir build && cd build
cmake ..
cmake --build .
./my_hunter
```

### Web (Emscripten)
```sh
source /path/to/emsdk/emsdk_env.sh
mkdir build-web && cd build-web
emcmake cmake ..
emmake cmake --build .
python3 -m http.server 8080   # then open my_hunter.html
```

## Controls
- **Click** the start button to begin
- **Click a barrel** to knock it back and damage it
- **Click Gragas** (once he spawns, round 2+) to make him jump
- **ENTER** — manual barrel spawn (hold to spawn faster, see above)
- **SPACE** — force-spawn Gragas immediately if he doesn't exist yet
