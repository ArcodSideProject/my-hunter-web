#include "raylib.h"
#include "game.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static GameWorld world;

static void UpdateDrawFrame(void) {
    float dt = GetFrameTime();
    UpdateGameWorld(&world, dt);

    BeginDrawing();
    DrawGameWorld(&world);
    EndDrawing();
}

int main(void) {
    InitWindow(WIDTH, HEIGHT, "My Hunter — raylib web edition");
    SetTargetFPS(60);

    InitGameWorld(&world);
    LoadGameAssets(&world);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    UnloadGameAssets(&world);
    DestroyPhysicsWorld(&world);
    CloseWindow();
    return 0;
}
