#include "button.h"

#include <string.h>

#include "../core/events.h"
#include "../core/view.h"
#include "../core/world.h"
#include "../core/solid.h"

typedef struct Button {
    bool alive;
    bool pressed;
    uint16_t zone[BUTTON_ZONE_MAX];
    int zone_count;
} Button;

static Button buttons[BUTTON_MAX];
static int button_cnt;
static int pressed_count;

void button_reset(void)
{
    memset(buttons, 0, sizeof(buttons));
    button_cnt = 0;
    pressed_count = 0;
}

void button_place(const uint16_t *zone, int zone_count)
{
    if (button_cnt >= BUTTON_MAX) return;
    Button *b = &buttons[button_cnt];
    b->alive = true;
    b->pressed = false;
    int n = zone_count < BUTTON_ZONE_MAX ? zone_count : BUTTON_ZONE_MAX;
    memcpy(b->zone, zone, sizeof(uint16_t) * (size_t)n);
    b->zone_count = n;
    button_cnt++;
}

void button_tick(void)
{
    pressed_count = 0;
    for (int i = 0; i < button_cnt; i++) {
        Button *b = &buttons[i];
        if (!b->alive) continue;
        bool pressed = false;
        for (int z = 0; z < b->zone_count && !pressed; z++)
            pressed = solid_presses_button(b->zone[z]);
        if (pressed && !b->pressed) events_sound(SND_BUTTON, 0);
        if (!pressed && b->pressed) events_sound(SND_WRONG, 0);
        b->pressed = pressed;
        if (pressed) pressed_count++;
    }
}

int button_count(void) { return button_cnt; }
bool button_alive(int index)
{
    return index >= 0 && index < button_cnt && buttons[index].alive;
}
bool button_pressed(int index)
{
    return index >= 0 && index < button_cnt && buttons[index].pressed;
}
bool button_all_pressed(void)
{
    return button_cnt > 0 && pressed_count == button_cnt;
}
uint16_t button_zone_cell(int index, int cell_i)
{
    return buttons[index].zone[cell_i];
}


/* ------------------------------------------------------------------ */
/* View                                                                */
/* ------------------------------------------------------------------ */

void button_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor tint = shadow ? view_rgb(0, 0, 0) : view_rgb(255, 255, 255);
    for (int i = 0; i < button_cnt; i++) {
        if (!buttons[i].alive) continue;
        float x = (float)(sim_cell_x(buttons[i].zone[0]) * 16);
        float y = (float)(sim_cell_y(buttons[i].zone[0]) * 16);
        /* the idle button cycles sprButton (9 frames @ 8 fps); pressed it
         * holds the single-frame sprButtonPressed */
        int frame = (int)view_sprite_clock(LONGO_SPR_BUTTON) % 9;
        if (shadow) {
            view_sprite(0,
                        buttons[i].pressed ? LONGO_SPR_BUTTONPRESSED
                                           : LONGO_SPR_BUTTON,
                        buttons[i].pressed ? 0 : frame, x, y + 4.0f, 1.0f,
                        1.0f, 0.0f, tint, 1.0f);
        } else {
            view_sprite(VIEW_DEPTH_GROUND,
                        buttons[i].pressed ? LONGO_SPR_BUTTONPRESSED
                                           : LONGO_SPR_BUTTON,
                        buttons[i].pressed ? 0 : frame, x, y, 1.0f, 1.0f,
                        0.0f, tint, 1.0f);
        }
    }
}
