/*
 * Shared occupancy/solidity feature.
 *
 * Every cell-occupying object registers itself here (walls, holes, doors,
 * boxes, the dog's head and body segments, the house footprint) and every
 * consumer — the dog's movement probe, box pushes, button presses — asks
 * this module instead of special-casing object types.  Per-kind semantics
 * stay with the owning object (hole.c decides whether a hole still
 * blocks, dog.c decides whether a body segment is solid); this module owns
 * the map and the shared policies.
 */
#ifndef LONGO_SOLID_H
#define LONGO_SOLID_H

#include <stdbool.h>
#include <stdint.h>

#include "world.h" /* cell limits */

typedef enum SolidKind {
    SOLID_EMPTY = 0,
    SOLID_WALL,   /* static wall block */
    SOLID_GOAL,   /* house footprint */
    SOLID_HOLE,   /* open hole; a filled hole stops being solid */
    SOLID_DOOR,   /* removed from the map when it poofs */
    SOLID_BOX,
    SOLID_BODY,   /* dog chain segment; the tail is not solid */
    SOLID_HEAD
} SolidKind;

/* Cell -> occupant entry; defined here so the undo history can capture
 * the whole map verbatim (core/undo.c).  Every cell also carries a
 * shadow: the entry a placement displaced, restored when the same owner
 * vacates (a box parked on a filled hole takes the cell from the hole
 * and hands it back when it moves on). */
typedef struct SolidCell {
    SolidKind kind;
    int index;
} SolidCell;

typedef struct SolidSnapshot {
    SolidCell cells[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];
    SolidCell shadow[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];
} SolidSnapshot;

void solid_reset(void);

/* Undo capture/restore. */
void solid_capture(SolidSnapshot *out);
void solid_restore(const SolidSnapshot *snap);

void solid_place(uint16_t cell, SolidKind kind, int index);
void solid_clear(uint16_t cell);
/* Owner-checked removal: clears the cell only when its current entry IS
 * the caller's (kind, index), restoring the shadowed entry.  Movers'
 * unstamp paths go through this; raw solid_clear stays for load-time
 * stamping and full resets. */
void solid_clear_owned(uint16_t cell, SolidKind kind, int index);

SolidKind solid_kind_at(uint16_t cell);
int solid_index_at(uint16_t cell);

/* The dog's movement probe: free, blocked, or occupied by a box (which
 * the dog may push — see box_push()). */
typedef enum SolidProbe {
    SOLID_PROBE_FREE = 0,
    SOLID_PROBE_SOLID,
    SOLID_PROBE_BOX
} SolidProbe;

SolidProbe solid_probe(uint16_t cell);

/* Can a pushed box land on this cell?  (Walls, doors, closed holes, the
 * house, body butts and other boxes say no; the tail and the head's cell
 * resolve through the movement rules.) */
bool solid_blocks_box(uint16_t cell);

/* Does the occupant of this cell press a button? */
bool solid_presses_button(uint16_t cell);

#endif /* LONGO_SOLID_H */
