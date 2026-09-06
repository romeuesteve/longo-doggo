/*
 * Door object script: solid until every button is pressed simultaneously;
 * then it plays its open squash for a few ticks and poofs away.
 */
#ifndef LONGO_OBJECT_DOOR_H
#define LONGO_OBJECT_DOOR_H

#include <stdbool.h>
#include <stdint.h>

#define DOOR_MAX 8

void door_reset(void);
void door_place(uint16_t cell);
void door_tick(bool all_buttons_pressed);

bool door_alive(int index);
bool door_open(int index);

/* View. */
void door_view_tick(void);
void door_draw(int shadow);

#endif /* LONGO_OBJECT_DOOR_H */
