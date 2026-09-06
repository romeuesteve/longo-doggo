#include "solid.h"

#include <string.h>

#include "../objects/dog.h"
#include "../objects/hole.h"

static SolidCell cells[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];

void solid_reset(void) { memset(cells, 0, sizeof(cells)); }

void solid_capture(SolidSnapshot *out)
{
    memcpy(out->cells, cells, sizeof(cells));
}

void solid_restore(const SolidSnapshot *snap)
{
    memcpy(cells, snap->cells, sizeof(cells));
}

void solid_place(uint16_t cell, SolidKind kind, int index)
{
    cells[cell].kind = kind;
    cells[cell].index = index;
}

void solid_clear(uint16_t cell)
{
    cells[cell].kind = SOLID_EMPTY;
    cells[cell].index = 0;
}

SolidKind solid_kind_at(uint16_t cell) { return cells[cell].kind; }

int solid_index_at(uint16_t cell) { return cells[cell].index; }

SolidProbe solid_probe(uint16_t cell)
{
    switch (cells[cell].kind) {
    case SOLID_EMPTY:
        return SOLID_PROBE_FREE;
    case SOLID_BOX:
        return SOLID_PROBE_BOX;
    case SOLID_HOLE:
        /* a filled hole behaves like empty ground (sprite NONE upstream) */
        return hole_is_full(cells[cell].index) ? SOLID_PROBE_FREE
                                               : SOLID_PROBE_SOLID;
    case SOLID_BODY:
        /* per-kind semantics stay with the owner: the tail part is not
         * solid, so the dog (and a pushed box) may take its cell; it
         * vacates in the same tick */
        return dog_part_is_solid(cells[cell].index) ? SOLID_PROBE_SOLID
                                                    : SOLID_PROBE_FREE;
    case SOLID_WALL:
    case SOLID_GOAL:
    case SOLID_DOOR:
    default:
        return SOLID_PROBE_SOLID;
    }
}

bool solid_blocks_box(uint16_t cell)
{
    switch (cells[cell].kind) {
    case SOLID_EMPTY:
        return false;
    case SOLID_HOLE:
        /* an open hole swallows a box; a filled one is normal ground */
        return !hole_is_full(cells[cell].index);
    case SOLID_BODY:
        /* same tail exception as the movement probe */
        return dog_part_is_solid(cells[cell].index);
    case SOLID_WALL:
    case SOLID_GOAL:
    case SOLID_DOOR:
    case SOLID_BOX:
    case SOLID_HEAD:
    default:
        return true;
    }
}

bool solid_presses_button(uint16_t cell)
{
    switch (cells[cell].kind) {
    case SOLID_HOLE:
        return !hole_is_full(cells[cell].index);
    case SOLID_BOX:
    case SOLID_BODY:
    case SOLID_HEAD:
    case SOLID_DOOR:
        return true;
    case SOLID_WALL:
    case SOLID_GOAL:
    case SOLID_EMPTY:
    default:
        return false;
    }
}
