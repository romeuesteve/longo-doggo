#include "solid.h"

#include <string.h>

#include "../objects/box.h"
#include "../objects/door.h"
#include "../objects/dog.h"
#include "../objects/hole.h"

static SolidCell cells[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];
/* What each cell restores to when its current occupant vacates (see
 * solid_place / solid_clear_owned). */
static SolidCell shadow[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];

void solid_reset(void)
{
    memset(cells, 0, sizeof(cells));
    memset(shadow, 0, sizeof(shadow));
}

void solid_capture(SolidSnapshot *out)
{
    memcpy(out->cells, cells, sizeof(cells));
    memcpy(out->shadow, shadow, sizeof(shadow));
}

void solid_restore(const SolidSnapshot *snap)
{
    memcpy(cells, snap->cells, sizeof(cells));
    memcpy(shadow, snap->shadow, sizeof(shadow));
}

void solid_place(uint16_t cell, SolidKind kind, int index)
{
    /* A same-kind placement is the same owner refreshing its entry (the
     * dog re-stamps its whole chain every step, with shifted positional
     * indices), so the shadow keeps what was under the owner instead of
     * the owner's own previous stamp. */
    if (cells[cell].kind != kind) shadow[cell] = cells[cell];
    cells[cell].kind = kind;
    cells[cell].index = index;
}

void solid_clear(uint16_t cell)
{
    cells[cell].kind = SOLID_EMPTY;
    cells[cell].index = 0;
}

/* A shadowed entry may have gone stale while its cell was overlaid (a
 * body index shifts along with the chain, a box can die in a hole): only
 * restore occupants that are still there.  Walls, the goal, holes and
 * the load-time stamps never move, so they restore unconditionally. */
static bool shadow_still_valid(uint16_t cell, const SolidCell *under)
{
    switch (under->kind) {
    case SOLID_BOX:
        return box_alive(under->index) && box_cell(under->index) == cell;
    case SOLID_DOOR:
        return door_alive(under->index) && door_cell(under->index) == cell;
    case SOLID_BODY:
        return under->index >= 0 && under->index < dog_length() &&
               dog_part_cell(under->index) == cell;
    case SOLID_HEAD:
        return dog_alive() && sim_cell_of(dog_cx(), dog_cy()) == cell;
    case SOLID_WALL:
    case SOLID_GOAL:
    case SOLID_HOLE:
    default:
        return true;
    }
}

void solid_clear_owned(uint16_t cell, SolidKind kind, int index)
{
    SolidCell under;
    if (cells[cell].kind != kind || cells[cell].index != index) return;
    under = shadow[cell];
    solid_clear(cell);
    shadow[cell].kind = SOLID_EMPTY;
    shadow[cell].index = 0;
    if (under.kind != SOLID_EMPTY && shadow_still_valid(cell, &under))
        cells[cell] = under;
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
