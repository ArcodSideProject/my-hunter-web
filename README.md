# My Hunter — raylib web edition

A recreation of Epitech's classic tek1 "MyHunter" wave-survival shooter,
rebuilt in C + [raylib](https://www.raylib.com) targeting native desktop
**and** the web (via Emscripten/WebAssembly), so it can be deployed on
a VPS and served through Cloudflare instead of requiring a local CSFML
install.

## Status

This is a from-scratch recreation based on the genre conventions of the
Epitech "Hunter" project family (wave-based top-down survival shooter,
auto/aimed shooting, leveling, boss waves) — **not** a line-for-line port
of a specific private `my_hunter` repository, since that repo wasn't
accessible from this environment. Update `src/game.c` to match your
original mechanics/balance once you're back.

**Build verification status:** built and run successfully in this
environment. `libraylib.a` was compiled from source (raylib 5.5) and
`src/*.c` was compiled + linked into a real native `aarch64` executable,
which was then run against a virtual framebuffer (Xvfb) — raylib
initialized OpenGL (Mesa/llvmpipe software rendering), compiled its
shaders, loaded its default font/texture, and reached the main game
loop (`Target time per frame: 16.667 milliseconds` in the log, meaning
`InitGameWorld`/`UpdateGameWorld`/`DrawGameWorld` all ran without
crashing). No system packages were installed (no sudo available) —
dev headers/libs were extracted locally from downloaded RPMs
(`dnf5 download`, no root required) into a throwaway prefix.

**Not yet verified:** the actual web/Emscripten build (needs emsdk +
a wasm-targeted raylib build, not attempted here) and real visual
gameplay/balance testing on a real display — the headless run confirms
the code *runs*, not that gameplay feel/balance is tuned right.

## Gameplay

- WASD / arrow keys to move
- Auto-fires at the nearest enemy
- Kill enemies for XP, level up to pick an upgrade (damage / fire rate /
  speed + max HP)
- Every 5th wave ends in a boss
- Survive as long as possible; score and wave reached are tracked

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
  game.h    — game state structs, constants
  game.c    — all gameplay logic (movement, spawning, combat, XP, HUD)
web/
  shell.html — Emscripten HTML shell (canvas + loading UI)
CMakeLists.txt — builds either native or web depending on -DEMSCRIPTEN
```

## License

zlib/libpng, matching raylib's own license, for easy static linking.
