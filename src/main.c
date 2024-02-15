#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define TARGET_FPS 100
#define W 400
#define H 200

#define cell_state_dead 0
#define cell_state_alive 1
#define cell_state_none 2

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

int seed;
char universe[W * H];

Texture2D gpu_data;
Color cpu_data[W * H];

static char cell_get(int x, int y)
{
    return universe[((y + H) % H) * W + ((x + W) % W)];
}

static void cell_set(int x, int y, char value)
{
    universe[((y + H) % H) * W + ((x + W) % W)] = value;
}

static int cell_tick(int x, int y)
{
    int r1, c1;
    int n = 0;
    for (r1 = y - 1; r1 <= y + 1; r1++) {
        for (c1 = x - 1; c1 <= x + 1; c1++) {
            if (cell_get(c1, r1) == cell_state_alive) {
                n++;
            }
        }
    }

    int was_alive = 0;
    if (cell_get(x, y) == cell_state_alive) {
        was_alive = 1;
        n--;
    }

    int cell_state = cell_get(x, y);
    int is_alive = (n == 3 || (n == 2 && cell_state == cell_state_alive));
    if (was_alive && !is_alive) {
        cell_state = cell_state_dead;
    } else if (is_alive) {
        cell_state = cell_state_alive;
    }

    return cell_state;
}

static Color cell_get_color(int x, int y)
{
    switch (cell_get(x, y)) {
    case cell_state_dead:
        return WHITE;
    case cell_state_alive:
        //return BLACK;
        return ColorFromHSV(CLAMP(x + y, 40, 360), 1.0, 1.0);
    case cell_state_none:
        return WHITE;
    default:
        return WHITE;
    }
}

static void universe_init()
{
    int y, x;
    srand(seed);
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            if (rand() < RAND_MAX / 2) {
                cell_set(x, y, cell_state_alive);
            } else {
                cell_set(x, y, cell_state_none);
            }
        }
    }
}

static void universe_tick()
{
    static char new[W * H];
    int y, x;
    int cell_state;

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            cell_state = cell_tick(x, y);
            new[y * W + x] = cell_state;
        }
    }

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            cell_state = new[y * W+ x];
            cell_set(x, y, cell_state);
        }
    }

}

static void universe_draw()
{
    int y, x;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            cpu_data[y * W + x] = cell_get_color(x, y);
        }
    }
}

void main_loop_body()
{
    if (IsKeyPressed(KEY_F) || IsGestureDetected(GESTURE_TAP)) {
        if (IsWindowFullscreen()) {
            RestoreWindow();
        } else {
            ToggleBorderlessWindowed();
        }
    }

    universe_tick();

    universe_draw();
    BeginDrawing();
        ClearBackground(WHITE);
        UpdateTexture(gpu_data, cpu_data);
        Rectangle source = {0, 0, W, H};
        Rectangle dest = {0, 0, GetRenderWidth(), GetRenderHeight()};
        Vector2 origin = {0, 0};
        DrawTexturePro(gpu_data, source, dest, origin, 0.0, WHITE);
        DrawFPS(10, 10);
    EndDrawing();
}

int main(int argc, char * argv[])
{
    InitWindow(W, H, "Conway's Game of Life");

    Image img = {
        .width = W,
        .height = H,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    img.data = (Color *)malloc(W * H * sizeof(Color));
    memcpy(img.data, cpu_data, (W * H * sizeof(Color)));
    gpu_data = LoadTextureFromImage(img);
    UnloadImage(img);

    seed = time(NULL);
    universe_init();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(main_loop_body, 120, 1);
#else

    SetTargetFPS(TARGET_FPS);
    DisableCursor();
    ToggleBorderlessWindowed();
    while (!WindowShouldClose()) {
        main_loop_body();
    }
    EnableCursor();
#endif

    CloseWindow();
    return 0;
}
