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
    uint16_t box_zone[BUTTON_ZONE_MAX];
    int box_zone_count;
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

void button_place(const uint16_t *zone, int zone_count,
                  const uint16_t *box_zone, int box_zone_count)
{
    if (button_cnt >= BUTTON_MAX) return;
    Button *b = &buttons[button_cnt];
    b->alive = true;
    b->pressed = false;
    int n = zone_count < BUTTON_ZONE_MAX ? zone_count : BUTTON_ZONE_MAX;
    memcpy(b->zone, zone, sizeof(uint16_t) * (size_t)n);
    b->zone_count = n;
    n = box_zone_count < BUTTON_ZONE_MAX ? box_zone_count : BUTTON_ZONE_MAX;
    memcpy(b->box_zone, box_zone, sizeof(uint16_t) * (size_t)n);
    b->box_zone_count = n;
    button_cnt++;
}

void button_tick(void)
{
    pressed_count = 0;
    for (int i = 0; i < button_cnt; i++) {
        Button *b = &buttons[i];
        if (!b->alive) continue;
        bool pressed = false;
        /* boxes press through the lid zone; everything else through the
         * body zone */
        for (int z = 0; z < b->box_zone_count && !pressed; z++)
            pressed = solid_kind_at(b->box_zone[z]) == SOLID_BOX;
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
uint16_t button_box_zone_cell(int index, int cell_i)
{
    return buttons[index].box_zone[cell_i];
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
        int pressed_sprite = buttons[i].pressed ? 1 : 0;
        if (shadow) {
            view_sprite(0, i, pressed_sprite ? LONGO_SPR_BUTTONPRESSED
                                             : LONGO_SPR_BUTTON,
                        0, x, y + 4.0f, 1.0f, 1.0f, 0.0f, tint, 1.0f);
        } else {
            view_sprite(200, i, pressed_sprite ? LONGO_SPR_BUTTONPRESSED
                                               : LONGO_SPR_BUTTON,
                        0, x, y, 1.0f, 1.0f, 0.0f, tint, 1.0f);
        }
    }
}
