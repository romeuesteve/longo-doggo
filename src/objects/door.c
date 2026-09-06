#include "door.h"

#include <string.h>

#include "../core/events.h"
#include "../core/view.h"
#include "../core/solid.h"
#include "../core/world.h"

/* The open animation eases scale 1 -> 1.2 at 0.1/tick and poofs past 1.15;
 * the cell stays solid for those 14 ticks. */
#define DOOR_OPEN_TICKS 14

static Door doors[DOOR_MAX];
static int door_cnt;

void door_reset(void)
{
    memset(doors, 0, sizeof(doors));
    door_cnt = 0;
}

void door_capture(DoorSnapshot *out)
{
    memcpy(out->doors, doors, sizeof(doors));
    out->count = door_cnt;
}

void door_restore(const DoorSnapshot *snap)
{
    memcpy(doors, snap->doors, sizeof(doors));
    door_cnt = snap->count;
}

void door_place(uint16_t cell)
{
    if (door_cnt >= DOOR_MAX) return;
    doors[door_cnt].alive = true;
    doors[door_cnt].open = false;
    doors[door_cnt].open_timer = 0;
    doors[door_cnt].cell = cell;
    solid_place(cell, SOLID_DOOR, door_cnt);
    door_cnt++;
}

void door_tick(bool all_buttons_pressed)
{
    for (int i = 0; i < door_cnt; i++) {
        Door *d = &doors[i];
        if (!d->alive) continue;
        if (!d->open && all_buttons_pressed) {
            d->open = true;
            d->open_timer = DOOR_OPEN_TICKS;
        }
        if (d->open) {
            if (--d->open_timer <= 0) {
                d->alive = false;
                solid_clear(d->cell);
                events_fx(FX_SMOKE_BURST,
                          (float)(sim_cell_x(d->cell) * SIM_CELL),
                          (float)(sim_cell_y(d->cell) * SIM_CELL), 0, 0, 0, 7,
                          0);
                events_sound(SND_POOF, 0);
            }
        }
    }
}

bool door_alive(int index)
{
    return index >= 0 && index < door_cnt && doors[index].alive;
}
bool door_open(int index)
{
    return index >= 0 && index < door_cnt && doors[index].open;
}


/* ------------------------------------------------------------------ */
/* View: open squash (scale ease 0.1; the sim poofs after 14 ticks)    */
/* ------------------------------------------------------------------ */

typedef struct DoorView {
    float scale_x, scale_y;
    float x, y;
} DoorView;

static DoorView v_doors[DOOR_MAX];

void door_view_tick(void)
{
    for (int i = 0; i < door_cnt; i++) {
        DoorView *v = &v_doors[i];
        if (!doors[i].alive) continue;
        if (!doors[i].open) {
            v->scale_x = v->scale_y = 1.0f;
            v->x = (float)(sim_cell_x(doors[i].cell) * SIM_CELL);
            v->y = (float)(sim_cell_y(doors[i].cell) * SIM_CELL);
            continue;
        }
        v->scale_x += (1.2f - v->scale_x) * 0.1f;
        v->scale_y += (0.8f - v->scale_y) * 0.1f;
        float sw = 16.0f * v->scale_x;
        float sh = 16.0f * v->scale_y;
        v->x = (float)(sim_cell_x(doors[i].cell) * SIM_CELL) -
               (sw * (v->scale_x - 1.0f)) / 2.0f;
        v->y = (float)(sim_cell_y(doors[i].cell) * SIM_CELL) -
               sh * (v->scale_y - 1.0f);
    }
}

void door_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor tint = shadow ? view_rgb(0, 0, 0) : view_rgb(255, 255, 255);
    for (int i = 0; i < door_cnt; i++) {
        if (!doors[i].alive) continue;
        if (shadow) {
            /* the door's shadow sits at its live x/y (it shifts during
             * the open squash), flipped below the origin: sprDoor at
             * (x, y + 22) with yscale -0.4 */
            view_sprite(0, LONGO_SPR_DOOR, 0, v_doors[i].x,
                        v_doors[i].y + 22.0f, 1.0f, -0.4f, 0.0f, tint, 1.0f);
        } else {
            view_sprite(VIEW_DEPTH_ITEM, LONGO_SPR_DOOR, 0, v_doors[i].x,
                        v_doors[i].y, v_doors[i].scale_x, v_doors[i].scale_y,
                        0.0f, tint, 1.0f);
        }
    }
}
