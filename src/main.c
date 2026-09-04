/*
 * Longo Doggo front-end.
 *
 * Runs the simulation at the original 60 FPS room speed (one tick per
 * vsynced frame, like GameMaker) and maps raylib input onto the sim's
 * held-direction + pressed-edge model.  Movement repeat lives in the
 * simulation (SIM_STEP_INTERVAL); the front-end no longer emulates OS
 * key repeat.  After each tick it advances the presentation and plays
 * the sounds the simulation queued.
 */
#include "core/world.h"
#include "render.h"

#include <stdbool.h>
#include <string.h>

typedef struct LongoFront {
    SimWorld world;
    LongoRender render;
} LongoFront;

int main(void)
{
    static LongoFront front;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(LONGO_LOGICAL_WIDTH * LONGO_WINDOW_SCALE,
               LONGO_LOGICAL_HEIGHT * LONGO_WINDOW_SCALE, "Longo Doggo");
    memset(&front, 0, sizeof(front));
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    if (!longo_render_init(&front.render, NULL)) {
        CloseWindow();
        return 1;
    }

    sim_init((unsigned int)GetRandomValue(0, 0x7fffffff));

    while (!WindowShouldClose()) {
        SimInput input;
        memset(&input, 0, sizeof(input));

        /* held directions: arrows and WASD merged (the sim applies the
         * original's horizontal-first diagonal priority) */
        input.held_right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
        input.held_left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
        input.held_down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
        input.held_up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);

        input.pressed_space = IsKeyPressed(KEY_SPACE);
        input.pressed_enter =
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        input.pressed_e = IsKeyPressed(KEY_E);
        input.pressed_r = IsKeyPressed(KEY_R);

        input.pressed_any = 0;
        for (int k = 0; k < 512; k++) {
            if (IsKeyPressed(k)) {
                input.pressed_any = 1;
                break;
            }
        }
        /* any-key starts the game; exit key is disabled */
        if (IsKeyPressed(KEY_ESCAPE)) input.pressed_any = 0;

        sim_tick(world_ptr(), &input);
        world_view_tick(&input);
        world_draw();
        longo_render_dispatch_sounds(&front.render, world_ptr());
        longo_render_frame(&front.render, world_ptr());
    }

    longo_render_shutdown(&front.render);
    CloseWindow();
    return 0;
}
