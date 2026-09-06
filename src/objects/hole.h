/*
 * Hole object script: a cell the dog cannot cross until a box fills it.
 */
#ifndef LONGO_OBJECT_HOLE_H
#define LONGO_OBJECT_HOLE_H

#include <stdbool.h>
#include <stdint.h>

#define HOLE_MAX 32

void hole_reset(void);
void hole_place(uint16_t cell);
bool hole_is_full(int index);
/* A box landing here fills the hole (the box is consumed by the caller). */
void hole_fill(int index);

int hole_count(void);
int hole_index_at(uint16_t cell); /* -1 when none */

/* Draw: sprHole frame 0 empty, 1 filled, depth 200.
 * A floor decal — it never contributes to the shadow pass. */
void hole_draw(void);

#endif /* LONGO_OBJECT_HOLE_H */
