/*
 * Box object script: pushable cargo.  The dog script asks box_push();
 * the landing-cell rules consult the shared solid feature.
 */
#ifndef LONGO_OBJECT_BOX_H
#define LONGO_OBJECT_BOX_H

#include <stdbool.h>
#include <stdint.h>

#define BOX_MAX 32

void box_reset(void);
void box_place(uint16_t cell);
void box_set_cell(int index, uint16_t cell); /* room edit + test hook */

int box_count(void);
bool box_alive(int index);
uint16_t box_cell(int index);
int box_index_at(uint16_t cell); /* -1 when none */

/* Attempt to push the box one cell; fills an open hole on the landing
 * cell (consuming the box), refuses when the landing cell is blocked.
 * Emits the push/poof sounds and the sink effect. */
bool box_push(int index, uint16_t from_cell, int dir);

/* View. */
void box_view_tick(void);
void box_draw(int shadow);
float box_visual_x(int index);
float box_visual_y(int index);

#endif /* LONGO_OBJECT_BOX_H */
