#include "hole.h"

#include <string.h>

#include "../core/solid.h"

typedef struct Hole {
    bool alive;
    bool full;
    uint16_t cell;
} Hole;

static Hole holes[HOLE_MAX];
static int hole_cnt;

void hole_reset(void)
{
    memset(holes, 0, sizeof(holes));
    hole_cnt = 0;
}

void hole_place(uint16_t cell)
{
    if (hole_cnt >= HOLE_MAX) return;
    holes[hole_cnt].alive = true;
    holes[hole_cnt].full = false;
    holes[hole_cnt].cell = cell;
    solid_place(cell, SOLID_HOLE, hole_cnt);
    hole_cnt++;
}

bool hole_is_full(int index)
{
    if (index < 0 || index >= hole_cnt) return false;
    return holes[index].full;
}

void hole_fill(int index)
{
    if (index < 0 || index >= hole_cnt) return;
    holes[index].full = true;
}

int hole_count(void) { return hole_cnt; }

uint16_t hole_cell(int index) { return holes[index].cell; }

int hole_index_at(uint16_t cell)
{
    for (int i = 0; i < hole_cnt; i++)
        if (holes[i].alive && holes[i].cell == cell) return i;
    return -1;
}
