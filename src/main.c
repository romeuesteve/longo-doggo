/*
 * Longo Doggo front-end.
 *
 * Runs the simulation at the original 60 FPS room speed (one tick per
 * frame, vsynced like GameMaker), maps raylib input onto GameMaker's
 * keyboard_check_pressed semantics (including OS-style key repeat), and
 * dispatches the simulation's sound queue.
 */
#include "game.h"
#include "render.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LONGO_KEY_REPEAT_DELAY 0.5f  /* Windows initial repeat delay */
#define LONGO_KEY_REPEAT_RATE (1.0f / 30.0f) /* ~30 Hz held repeat */

typedef struct KeyChannel {
    bool down;          /* physical state this frame */
    bool pressed;       /* keyboard_check_pressed() edge for this tick */
    float held;
} KeyChannel;

typedef struct LongoFront {
    LongoWorld world;
    LongoRender render;
    KeyChannel keys[12];
} LongoFront;

enum {
    CH_RIGHT = 0, CH_LEFT, CH_UP, CH_DOWN,
    CH_D, CH_A, CH_S, CH_W,
    CH_R, CH_SPACE, CH_ENTER, CH_E,
    CH_COUNT
};

static bool key_down(int code)
{
    switch (code) {
    case CH_RIGHT: return IsKeyDown(KEY_RIGHT);
    case CH_LEFT: return IsKeyDown(KEY_LEFT);
    case CH_UP: return IsKeyDown(KEY_UP);
    case CH_DOWN: return IsKeyDown(KEY_DOWN);
    case CH_D: return IsKeyDown(KEY_D);
    case CH_A: return IsKeyDown(KEY_A);
    case CH_S: return IsKeyDown(KEY_S);
    case CH_W: return IsKeyDown(KEY_W);
    case CH_R: return IsKeyDown(KEY_R);
    case CH_SPACE: return IsKeyDown(KEY_SPACE);
    case CH_ENTER: return IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_KP_ENTER);
    case CH_E: return IsKeyDown(KEY_E);
    default: return false;
    }
}

static void update_channel(KeyChannel *ch, float dt, bool down)
{
    ch->pressed = false;
    if (down) {
        if (!ch->down) {
            ch->pressed = true;
            ch->held = 0.0f;
        } else {
            ch->held += dt;
            /* GameMaker surfaces OS key repeat through keyboard_check_pressed */
            if (ch->held >= LONGO_KEY_REPEAT_DELAY) {
                ch->held -= LONGO_KEY_REPEAT_RATE;
                ch->pressed = true;
            }
        }
    }
    ch->down = down;
}

int main(void)
{
    /* The world pool is several megabytes: keep it out of the stack. */
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
    longo_init(&front.world, (unsigned int)GetRandomValue(0, 0x7fffffff));

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        LongoInput input;
        memset(&input, 0, sizeof(input));

        for (int i = 0; i < CH_COUNT; i++) {
            update_channel(&front.keys[i], dt, key_down(i));
        }
        input.vk_right = front.keys[CH_RIGHT].pressed;
        input.vk_left = front.keys[CH_LEFT].pressed;
        input.vk_up = front.keys[CH_UP].pressed;
        input.vk_down = front.keys[CH_DOWN].pressed;
        input.key_d = front.keys[CH_D].pressed;
        input.key_a = front.keys[CH_A].pressed;
        input.key_s = front.keys[CH_S].pressed;
        input.key_w = front.keys[CH_W].pressed;
        input.key_r = front.keys[CH_R].pressed;
        input.key_space = front.keys[CH_SPACE].pressed;
        input.key_enter = front.keys[CH_ENTER].pressed;
        input.key_e = front.keys[CH_E].pressed;
        input.vk_anykey = 0;
        for (int k = 0; k < 512; k++) {
            if (IsKeyPressed(k)) {
                input.vk_anykey = 1;
                break;
            }
        }

        /* editor (oMouse) input in GUI/room coordinates */
        input.mouse_x = (float)GetMouseX() / (float)LONGO_WINDOW_SCALE;
        input.mouse_y = (float)GetMouseY() / (float)LONGO_WINDOW_SCALE;
        input.mb_left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.mb_right = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        input.mouse_wheel = (int)GetMouseWheelMove();

        longo_tick(&front.world, &input, dt * 1000.0);
        longo_render_dispatch_sounds(&front.render, &front.world);
        longo_render_frame(&front.render, &front.world);
    }

    longo_render_shutdown(&front.render);
    CloseWindow();
    return 0;
}
