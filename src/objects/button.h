/*
 * Button object script: pressed by boxes (through the lid straddle cell)
 * or by any body/hole/door occupant of its cells; the doors listen for
 * "all pressed at once".
 */
#ifndef LONGO_OBJECT_BUTTON_H
#define LONGO_OBJECT_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_MAX 16
#define BUTTON_ZONE_MAX 16

void button_reset(void);
/* The zones are computed by the world's placement pass from the original
 * bbox geometry: body zone (16x16 probe) and box zone (+ the 4px lid). */
void button_place(const uint16_t *zone, int zone_count,
                  const uint16_t *box_zone, int box_zone_count);

void button_tick(void);

int button_count(void);
bool button_alive(int index);
bool button_pressed(int index);
bool button_all_pressed(void);
uint16_t button_zone_cell(int index, int cell_i);
uint16_t button_box_zone_cell(int index, int cell_i);

#endif /* LONGO_OBJECT_BUTTON_H */
