#include "items.h"

#include <string.h>

typedef struct Item {
    bool alive;
    uint16_t cell;
} Item;

static Item apples[ITEMS_MAX];
static int apple_cnt;
static Item skulls[ITEMS_MAX];
static int skull_cnt;

void items_reset(void)
{
    memset(apples, 0, sizeof(apples));
    apple_cnt = 0;
    memset(skulls, 0, sizeof(skulls));
    skull_cnt = 0;
}

void apple_place(uint16_t cell)
{
    if (apple_cnt >= ITEMS_MAX) return;
    apples[apple_cnt].alive = true;
    apples[apple_cnt].cell = cell;
    apple_cnt++;
}

void skull_place(uint16_t cell)
{
    if (skull_cnt >= ITEMS_MAX) return;
    skulls[skull_cnt].alive = true;
    skulls[skull_cnt].cell = cell;
    skull_cnt++;
}

int apple_count(void) { return apple_cnt; }
bool apple_alive(int index)
{
    return index >= 0 && index < apple_cnt && apples[index].alive;
}
uint16_t apple_cell(int index) { return apples[index].cell; }
int apple_index_at(uint16_t cell)
{
    for (int i = 0; i < apple_cnt; i++)
        if (apples[i].alive && apples[i].cell == cell) return i;
    return -1;
}
void apple_consume(int index)
{
    if (index >= 0 && index < apple_cnt) apples[index].alive = false;
}

int skull_count(void) { return skull_cnt; }
bool skull_alive(int index)
{
    return index >= 0 && index < skull_cnt && skulls[index].alive;
}
uint16_t skull_cell(int index) { return skulls[index].cell; }
int skull_index_at(uint16_t cell)
{
    for (int i = 0; i < skull_cnt; i++)
        if (skulls[i].alive && skulls[i].cell == cell) return i;
    return -1;
}
void skull_consume(int index)
{
    if (index >= 0 && index < skull_cnt) skulls[index].alive = false;
}
