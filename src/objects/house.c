#include "house.h"

#include <string.h>

#include "../core/events.h"

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

void house_reset(void)
{
    memset(&house, 0, sizeof(house));
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
