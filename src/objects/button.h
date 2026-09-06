/*
 * Button object script: pressed by a box on its cell or by any
 * body/hole/door occupant; the doors listen for "all pressed at once".
 */
#ifndef LONGO_OBJECT_BUTTON_H
#define LONGO_OBJECT_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_MAX 16
#define BUTTON_ZONE_MAX 16

void button_reset(void);
/* The zone is computed by the world's placement pass: the cells whose
 * 16px probe rect strictly overlaps the button's footprint. */
void button_place(const uint16_t *zone, int zone_count);

void button_tick(void);

int button_count(void);
bool button_alive(int index);
bool button_pressed(int index);
bool button_all_pressed(void);
uint16_t button_zone_cell(int index, int cell_i);

/* View. */
void button_draw(int shadow);

#endif /* LONGO_OBJECT_BUTTON_H */
