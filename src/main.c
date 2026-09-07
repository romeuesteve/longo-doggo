/*
 * Longo Doggo front-end.
 *
 * Fixed-step orchestrator: every rendered frame samples the input edges
 * once and hands them to the shared schedule (sim_frame), which runs
 * zero or more 1/60 s updates — one rules tick plus one view pass per
 * update, each update's queued sounds delivered before the next update
 * clears the queues.  Catch-up is capped, so a long stall drops the
 * backlog and resumes at the next update instead of fast-forwarding.
 * Drawing happens once per rendered frame and never advances animation
 * time.  The native loop and the Emscripten loop share longo_frame().
 */
#include "core/world.h"
#include "render.h"

#include <stdbool.h>
#include <string.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

typedef struct LongoFront {
    SimWorld world;
    LongoRender render;
} LongoFront;

static LongoFront front;

static void deliver_sounds(void *user)
{
    longo_render_dispatch_sounds((LongoRender *)user, world_ptr());
}

static void longo_frame(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));

    /* movement is edge-triggered: one step per physical press, no
     * OS key repeat.  Edges are sampled once per rendered frame; the
     * schedule holds them until the first update that actually runs. */
    input.pressed_right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    input.pressed_left = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    input.pressed_down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    input.pressed_up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);

    input.pressed_space = IsKeyPressed(KEY_SPACE);
    input.pressed_enter =
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    input.pressed_e = IsKeyPressed(KEY_E);
    input.pressed_r = IsKeyPressed(KEY_R);
    input.pressed_undo = IsKeyPressed(KEY_Z); /* Ctrl+Z included — a held
                                               * Ctrl does not affect the
                                               * press edge */

    /* any key press starts the game; drain the press queue instead
     * of probing all 512 key slots */
    input.pressed_any = 0;
    while (GetKeyPressed() != 0) input.pressed_any = 1;
    /* exit key is disabled; escape does not start the game */
    if (IsKeyPressed(KEY_ESCAPE)) input.pressed_any = 0;

    /* run 0..SIM_MAX_CATCHUP updates of 1/60 s for the real frame time:
     * each update's sounds are dispatched before the next update runs */
    sim_frame((double)GetFrameTime() * 1000.0, &input, deliver_sounds,
              &front.render);

    /* one read-only draw + present per rendered frame */
    world_draw();
    longo_render_frame(&front.render, world_ptr());
}

int main(void)
{
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
    /* deliver the title room's load-time music request before the first
     * update's events_clear() wipes the queue (see title_place) */
    longo_render_dispatch_sounds(&front.render, world_ptr());

#if defined(__EMSCRIPTEN__)
    /* the browser drives the loop through requestAnimationFrame; this
     * never returns, so teardown happens when the tab closes */
    emscripten_set_main_loop(longo_frame, 0, 1);
#else
    while (!WindowShouldClose()) {
        longo_frame();
    }

    longo_render_shutdown(&front.render);
    CloseWindow();
#endif
    return 0;
}
