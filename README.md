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

**Not yet ported:** the original's actual sprite art (barrel/Gragas/
background PNGs) — this version draws primitive shapes (circles,
rounded rects) standing in for the real sprites, since the asset files
weren't pulled into this repo. Swap in the real `assets/` sprites from
the original repo for a visual match; the physics/logic layer is
already correct underneath.

**Build verification status:** both native and web builds compile and
run cleanly (`gcc -Wall -Wextra`, zero warnings; `emcc`, only a benign
pre-existing linker warning about an unused desktop-only object file).
Both were run and screenshotted: native via Xvfb + synthetic click
(confirmed barrels spawn, land, and Gragas engages), web via
`python3 -m http.server` + loaded in an actual browser.

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
