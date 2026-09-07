/*
 * Door object script: solid until every button is pressed simultaneously;
 * then it plays its open squash for a few ticks and poofs away.
 */
#ifndef LONGO_OBJECT_DOOR_H
#define LONGO_OBJECT_DOOR_H

#include <stdbool.h>
#include <stdint.h>

#define DOOR_MAX 8

/* State defined here so the undo history can capture it verbatim
 * (core/undo.c). */
typedef struct Door {
    bool alive;
    bool open;       /* all buttons pressed; solid until removed */
    int open_timer;
    uint16_t cell;
} Door;

typedef struct DoorSnapshot {
    Door doors[DOOR_MAX];
    int count;
} DoorSnapshot;

void door_reset(void);
void door_place(uint16_t cell);

/* Undo capture/restore. */
void door_capture(DoorSnapshot *out);
void door_restore(const DoorSnapshot *snap);

void door_tick(bool all_buttons_pressed);

bool door_alive(int index);
bool door_open(int index);
uint16_t door_cell(int index); /* read-only; 0 when out of range */

/* View. */
void door_view_tick(void);
void door_draw(int shadow);

#endif /* LONGO_OBJECT_DOOR_H */
