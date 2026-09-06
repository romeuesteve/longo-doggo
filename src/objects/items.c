#include "items.h"

#include <string.h>

#include "../core/view.h"
#include "../core/world.h"

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


/* ------------------------------------------------------------------ */
/* View                                                                */
/* ------------------------------------------------------------------ */

void items_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor white = view_rgb(255, 255, 255);
    ViewColor black = view_rgb(0, 0, 0);
    int apple_frame = (int)view_apple_clock() % 8;
    int pear_frame = (int)view_pear_clock() % 8;
    for (int i = 0; i < apple_cnt; i++) {
        if (!apples[i].alive) continue;
        float x = (float)(sim_cell_x(apples[i].cell) * 16);
        float y = (float)(sim_cell_y(apples[i].cell) * 16);
        if (shadow)
            view_sprite(0, i, LONGO_SPR_APPLE, apple_frame, x, y + 7.0f,
                        1.0f, 0.6f, 0.0f, black, 1.0f);
        else
            view_sprite(100, i, LONGO_SPR_APPLE, apple_frame, x, y, 1.0f,
                        1.0f, 0.0f, white, 1.0f);
    }
    for (int i = 0; i < skull_cnt; i++) {
        if (!skulls[i].alive) continue;
        float x = (float)(sim_cell_x(skulls[i].cell) * 16);
        float y = (float)(sim_cell_y(skulls[i].cell) * 16);
        /* oSkull's sprite in data.win is sprPear (8 frames @ 16 fps); the
         * sprSkull asset is never referenced by the object */
        if (shadow)
            view_sprite(0, i, LONGO_SPR_PEAR, pear_frame, x, y + 7.0f, 1.0f,
                        0.6f, 0.0f, black, 1.0f);
        else
            view_sprite(100, i, LONGO_SPR_PEAR, pear_frame, x, y, 1.0f, 1.0f,
                        0.0f, white, 1.0f);
    }
}
