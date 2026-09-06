/*
 * Items object script: apples (the dog grows) and skulls/pears (the dog
 * shrinks, or dies at minimum length).  Pure state + placement; the dog
 * script decides what eating means.
 */
#ifndef LONGO_OBJECT_ITEMS_H
#define LONGO_OBJECT_ITEMS_H

#include <stdbool.h>
#include <stdint.h>

#define ITEMS_MAX 32

/* State defined here so the undo history can capture it verbatim
 * (core/undo.c). */
typedef enum ItemKind { KIND_APPLE, KIND_SKULL, KIND_COUNT } ItemKind;

typedef struct Item {
    bool alive;
    uint16_t cell;
} Item;

typedef struct ItemsSnapshot {
    Item items[KIND_COUNT][ITEMS_MAX];
    int count[KIND_COUNT];
} ItemsSnapshot;

/* Undo capture/restore. */
void items_capture(ItemsSnapshot *out);
void items_restore(const ItemsSnapshot *snap);

void items_reset(void);
void apple_place(uint16_t cell);
void skull_place(uint16_t cell);

int apple_count(void);
bool apple_alive(int index);
uint16_t apple_cell(int index);
int apple_index_at(uint16_t cell); /* -1 when none */
void apple_consume(int index);

int skull_count(void);
bool skull_alive(int index);
uint16_t skull_cell(int index);
int skull_index_at(uint16_t cell); /* -1 when none */
void skull_consume(int index);

/* View. */
void items_draw(int shadow);

#endif /* LONGO_OBJECT_ITEMS_H */
