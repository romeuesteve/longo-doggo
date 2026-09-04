#include "house.h"

#include <string.h>

#include <stdio.h>

#include "../core/events.h"
#include "../core/view.h"
#include "../core/world.h"

typedef struct House {
    bool alive;
    uint16_t goal_cell;
    int remain;
    bool win_sound_played;
    bool win_alive;
    uint16_t win_zone[HOUSE_WIN_ZONE_MAX];
    int win_zone_count;
} House;

static House house;

/* view state: house pulse (oGoalUp count/count2 squash) */
static int v_count = 200, v_count2;
static float v_scale_x = 1.0f, v_scale_y = 1.0f;

void house_reset(void)
{
    memset(&house, 0, sizeof(house));
    v_count = 200;
    v_count2 = 0;
    v_scale_x = v_scale_y = 1.0f;
    /* oGoalUp create defaults remain to 1; the first tick recomputes it.
     * Initialising here prevents the win from arming on the load tick. */
    house.remain = 1;
}

void house_place_goal(uint16_t goal_cell)
{
    house.alive = true;
    house.goal_cell = goal_cell;
}

void house_place_win_zone(const uint16_t *cells, int count)
{
    if (count > HOUSE_WIN_ZONE_MAX) count = HOUSE_WIN_ZONE_MAX;
    memcpy(house.win_zone, cells, sizeof(uint16_t) * (size_t)count);
    house.win_zone_count = count;
    house.win_alive = true;
}

void house_tick(int dog_length)
{
    if (!house.alive) return;
    if (dog_length >= 0) house.remain = dog_length - 2;
    if (house.remain <= 0 && !house.win_sound_played) {
        events_sound(SND_WIN, 0);
        house.win_sound_played = true;
    }
}

bool house_alive(void) { return house.alive; }
uint16_t house_goal_cell(void) { return house.goal_cell; }
int house_remain(void) { return house.remain; }
bool house_win_ready(void) { return house.remain <= 0; }
bool house_win_alive(void) { return house.win_alive; }

uint16_t house_win_zone_cell(int index) { return house.win_zone[index]; }

bool house_try_win(uint16_t head_cell)
{
    if (!house.win_alive || !house.alive || !house_win_ready()) return false;
    for (int z = 0; z < house.win_zone_count; z++) {
        if (house.win_zone[z] == head_cell) {
            house.win_alive = false;
            return true;
        }
    }
    return false;
}


static float wave_pulse(int c)
{
    return longo_wave(-(float)c / 1000.0f, (float)c / 1000.0f, 0.35f, 0,
                      view_time_ms());
}

/* ------------------------------------------------------------------ */
/* View: house pulse + remain number                                   */
/* ------------------------------------------------------------------ */

void house_view_tick(void)
{
    if (!house.alive) return;
    float px = (float)(sim_cell_x(house.goal_cell) * SIM_CELL + 8);
    float py = (float)(sim_cell_y(house.goal_cell) * SIM_CELL + 8);
    int win_ready = house.remain <= 0;

    if (!win_ready) {
        if (v_count2 > 0) {
            if (v_count2 > 160)
                events_fx(FX_SMOKE_BURST,
                          px + world_random_range(-4.0f, 4.0f), py - 8.0f, 0,
                          0, 0, 1, 0);
            v_count2 -= 4;
            v_scale_x = 1.0f + wave_pulse(v_count2);
            v_scale_y = 1.0f - wave_pulse(v_count2);
        }
    } else {
        if (v_count > 0) {
            if (v_count > 160)
                events_fx(FX_SMOKE_BURST,
                          px + world_random_range(-4.0f, 4.0f), py - 8.0f, 0,
                          0, 0, 1, 0);
            v_count -= 4;
            v_scale_x = 1.0f + wave_pulse(v_count);
            v_scale_y = 1.0f - wave_pulse(v_count);
        }
        v_count2 = 200;
    }
}

void house_draw(int shadow)
{
    if (!house.alive) return;
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor tint = shadow ? view_rgb(0, 0, 0) : view_rgb(255, 255, 255);
    /* oGoal instance position: cell column centre, one cell below the top */
    float gx = (float)(sim_cell_x(house.goal_cell) * SIM_CELL + 8);
    float gy = (float)(sim_cell_y(house.goal_cell) * SIM_CELL + 16);

    if (shadow) {
        view_sprite(0, 0, LONGO_SPR_HOUSE, 0, gx, gy + 4.0f, 1.0f, 0.5f, 0.0f,
                    tint, 1.0f);
        return;
    }
    int frame = house.remain <= 0 ? 1 : 0;
    view_sprite_part(-180, 0, LONGO_SPR_HOUSE, frame, 0, 0, 64, 44,
                     gx - (32.0f * v_scale_x), gy - (64.0f * v_scale_y),
                     v_scale_x, v_scale_y, tint, 1.0f);
    if (house.remain > 0) {
        char text[16];
        float wave = longo_wave(-v_count2 / 50.0f, v_count2 / 50.0f, 0.35f, 0,
                                view_time_ms());
        float y = (gy + 1.0f) - 32.0f + wave;
        snprintf(text, sizeof(text), "%d", house.remain);
        view_text(-220, 1, 2, text, gx + 1.0f, y - 6.0f, 1.0f,
                  view_rgb(128, 0, 0));
        view_text(-220, 2, 2, text, gx, y - 6.0f, 1.0f, view_rgb(255, 0, 0));
    }
}
