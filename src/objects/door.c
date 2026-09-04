#include "door.h"

#include <string.h>

#include "../core/events.h"
#include "../core/solid.h"
#include "../core/world.h"

/* The open animation eases scale 1 -> 1.2 at 0.1/tick and poofs past 1.15
 * (14 ticks); the cell stays solid for that duration, like the original. */
#define DOOR_OPEN_TICKS 14

typedef struct Door {
    bool alive;
    bool open;       /* all buttons pressed; solid until removed */
    int open_timer;
    uint16_t cell;
} Door;

static Door doors[DOOR_MAX];
static int door_cnt;

void door_reset(void)
{
    memset(doors, 0, sizeof(doors));
    door_cnt = 0;
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

int door_count(void) { return door_cnt; }
bool door_alive(int index)
{
    return index >= 0 && index < door_cnt && doors[index].alive;
}
bool door_open(int index)
{
    return index >= 0 && index < door_cnt && doors[index].open;
}
uint16_t door_cell(int index) { return doors[index].cell; }

int door_index_at(uint16_t cell)
{
    for (int i = 0; i < door_cnt; i++)
        if (doors[i].alive && doors[i].cell == cell) return i;
    return -1;
}
