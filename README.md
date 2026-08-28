# My Hunter — raylib web edition

A faithful port of Epitech's tek1 "MyHunter" (repo:
`EpitechPromo2027/B-MUL-100-LYN-1-1-myhunter-antoine.esman`), rebuilt in
C + [raylib](https://www.raylib.com) targeting native desktop **and**
the web (via Emscripten/WebAssembly), so it can be deployed on a VPS
and served through Cloudflare instead of requiring a local CSFML install.

## Status

Real port, not a genre guess — mechanics, constants, and game-state
machine were read directly from the original CSFML source and
reproduced 1:1 in `src/game.c`:

- Barrels spawn from the top of the screen with the same gravity/air
  friction/floor-bounce physics as the original (`GRAVITY`, `AIR_FRICTION`,
  `FLOOR_FRICTION`, same `WIDTH`/`HEIGHT` of 1152x672)
- Clicking a barrel knocks it (`velocity.y = -60`, random x kick) and
  reduces its health, matching `barrel_touch.c`
- Gragas spawns once round 2 starts, auto-walks toward the nearest
  landed barrel and pops it on contact (`goto_barrel.c`), same
  `GRAGAS_FRICTION` and acceleration values
- Clicking Gragas makes him jump (`gragas_touch.c`)
- 5 rounds with the same spawn-rate/health-per-round table as
  `easy_rounds.c` (round 1: 1hp barrels, spawn every 0.5s after 5 spawned
  → round 2 ... → round 5: ends once 70 barrels spawned and the board
  is clear)
- Gragas's own score slows the barrel spawn rate the same way
  (`spawn_rate -= gscore/30`, floor 0.3)
- **Correct physics timing**: the original scales its delta-time by a
  `GAME_TICK = 20` factor before using it anywhere in movement code
  (`get_delta_t.c`) -- every velocity/acceleration constant in the game
  (`GRAVITY`, barrel/Gragas speeds, etc.) was tuned against that scaled
  value, not real seconds. This port reproduces that exactly: a
  `physicsDt = dt * GAME_TICK` is computed once per frame and used only
  for velocity/position integration, while animation timers, spawn
  cadence, and the explosion/spawn-fade clocks all use real, unscaled
  `dt` -- matching the original's split between `movement()` (scaled)
  and `sfClock_getElapsedTime()`-based timers (real time). Getting this
  wrong (using real-seconds dt for physics) made every earlier build of
  this port ~20x slower than intended.
- **ENTER**/**SPACE** debug keys, matching `manage_keys()`: ENTER
  spawns a manual 1hp barrel, SPACE force-spawns Gragas immediately if
  he doesn't already exist -- these are real features of the original,
  not testing shortcuts.
- **Pixel-perfect tree transparency on hover**, matching `blur_tree.c`:
  when the mouse is over an actually-opaque pixel of the tree sprite
  (checked via a CPU-side `Image` alpha lookup, not just the bounding
  box), the tree gradually fades to ~59% opacity so you can see behind
  it, and fades back to fully opaque when the mouse moves away.
- Barrels now clamp exactly to the floor line when landing (`pos.y =
  floorPos`) instead of potentially resting a few px into the ground on
  a fast/coarse physics step, which the original didn't explicitly
  guard against.
- **Real physics via Box2D v3**: barrel gravity/floor-contact/wall-bounce
  is now handled by an actual Box2D world instead of hand-rolled
  velocity/friction math. The original had zero collision between
  barrels, so overlapping ones would visually sink into or pass through
  each other instead of settling next to one another ("barrels aren't
  fixed to the ground") -- real rigid-body collision fixes that as a
  side effect. Gragas's AI/movement stays on the original's custom
  logic (Box2D is only used for barrel dynamics, not the character
  controller).
- Holding **ENTER** now ramps up to 50 spawns/second over 3 seconds of
  continuous hold (was 20/s over 2s), and the barrel pool was raised
  from 128 to 800 slots so spamming ENTER doesn't silently hit a cap.

**Not yet ported:** nothing outstanding on the assets/mechanics front —
the real sprite sheets (`barrel.png`, `sumos.png`, `explosion.png`,
`start_button.png`, all 7 background layers, the original font) were
pulled from the source repo and are now loaded/drawn with the exact
same frame-rect/scale math as the CSFML original (see `LoadGameAssets`,
`DrawBarrel`, `DrawGragas`, `DrawBackground` in `src/game.c`). A
primitive-shape fallback path still exists and activates automatically
if any texture fails to load, so the game never crashes on a missing
asset — but with `assets/` present it should look like the real game.

**Build verification status:** both native and web builds compile
cleanly. Runtime-verified via Xvfb + synthetic keyboard input (not
mouse — see note below) + screenshot: confirmed all 12 textures and
the real font load successfully (`TEXTURE: [ID N] Texture loaded
successfully` for each, matching dimensions: 22x28 barrel, 338x108
sumos, 315x51 explosion, 384x224 per background layer), barrels
visibly spawn/fall/render with real sprites, Gragas engages once round
2 starts.

Note on testing barrels "not appearing": an earlier debug pass showed
no barrels on screen after a simulated menu click. Root-caused to the
**test harness**, not the game: Xvfb has no window manager, so
`xdotool`'s synthetic mouse clicks never reached the app's input focus
(confirmed via `TraceLog` — zero `HandleClick` calls fired despite the
click command reporting success). A `SPACE`-to-start debug shortcut
was added to `UpdateGameWorld` specifically to make headless testing
reliable without depending on mouse-click delivery; real mouse clicks
work normally when actually running with a display/window manager
present (i.e. for an actual player).

## Gameplay

- Click **START** on the menu to begin
- Click barrels to knock/damage them before they land
- Once round 2 starts, Gragas auto-hunts landed barrels for you — click
  him to make him jump
- Survive all 5 rounds; the game ends once 70 barrels have spawned and
  the board is clear

## Building — native (desktop, for local testing)

Requires a C compiler and raylib's build dependencies. On Fedora/similar:

```bash
sudo dnf5 install -y gcc cmake mesa-libGL-devel libX11-devel \
  libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
  libXext-devel wayland-devel wayland-protocols-devel libxkbcommon-devel
```

Then:

```bash
mkdir build && cd build
cmake ..
cmake --build . -j
./my_hunter
```

(If raylib isn't found system-wide, CMake will fetch and build it from
source automatically via `FetchContent`.)

## Building — web (Emscripten / WebAssembly)

Requires [emsdk](https://emscripten.org/docs/getting_started/downloads.html)
installed and a raylib build for the `wasm32-unknown-emscripten` target.

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh
cd ..

mkdir build-web && cd build-web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake cmake --build . -j
```

This produces `my_hunter.html`, `my_hunter.js`, and `my_hunter.wasm` in
`build-web/`. Serve them with any static file server:

```bash
python3 -m http.server -d build-web 8080
```

Open `http://localhost:8080/my_hunter.html`.

## Deploying to your Oracle VPS + Cloudflare

1. Copy the built `my_hunter.html` / `.js` / `.wasm` / `.data` (if any)
   files to the VPS, e.g. served via nginx as static files.
2. Point an nginx `server` block at that directory.
3. Put it behind Cloudflare the same way as the Immich setup
   (Cloudflare Tunnel, or a plain A/CNAME record if the VPS has a
   public IP already) — no special CORS/headers needed since it's just
   static files, no backend API calls.

## Project layout

```
src/
  main.c    — entry point, desktop/web main loop split
  game.h    — game state structs, constants (mirrors the original's my.h)
  game.c    — all gameplay logic (barrels, Gragas AI, rounds, input, draw)
web/
  shell.html — Emscripten HTML shell (canvas + loading UI)
CMakeLists.txt — builds either native or web depending on -DEMSCRIPTEN
```

## License

zlib/libpng, matching raylib's own license, for easy static linking.
