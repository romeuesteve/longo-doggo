#include "items.h"

#include <string.h>

#include "../core/view.h"
#include "../core/world.h"

static Item items[KIND_COUNT][ITEMS_MAX];
static int item_cnt[KIND_COUNT];

void items_reset(void)
{
    memset(items, 0, sizeof(items));
    memset(item_cnt, 0, sizeof(item_cnt));
}

void items_capture(ItemsSnapshot *out)
{
    memcpy(out->items, items, sizeof(items));
    memcpy(out->count, item_cnt, sizeof(item_cnt));
}

void items_restore(const ItemsSnapshot *snap)
{
    memcpy(items, snap->items, sizeof(items));
    memcpy(item_cnt, snap->count, sizeof(item_cnt));
}

static void item_place(int kind, uint16_t cell)
{
    if (item_cnt[kind] >= ITEMS_MAX) return;
    items[kind][item_cnt[kind]].alive = true;
    items[kind][item_cnt[kind]].cell = cell;
    item_cnt[kind]++;
}

void apple_place(uint16_t cell) { item_place(KIND_APPLE, cell); }
void pear_place(uint16_t cell) { item_place(KIND_PEAR, cell); }

static int item_count(int kind) { return item_cnt[kind]; }

static bool item_alive(int kind, int index)
{
    return index >= 0 && index < item_cnt[kind] && items[kind][index].alive;
}

static uint16_t item_cell(int kind, int index)
{
    return items[kind][index].cell;
}

static int item_index_at(int kind, uint16_t cell)
{
    for (int i = 0; i < item_cnt[kind]; i++)
        if (items[kind][i].alive && items[kind][i].cell == cell) return i;
    return -1;
}

static void item_consume(int kind, int index)
{
    if (index >= 0 && index < item_cnt[kind]) items[kind][index].alive = false;
}

int apple_count(void) { return item_count(KIND_APPLE); }
bool apple_alive(int index) { return item_alive(KIND_APPLE, index); }
uint16_t apple_cell(int index) { return item_cell(KIND_APPLE, index); }
int apple_index_at(uint16_t cell) { return item_index_at(KIND_APPLE, cell); }
void apple_consume(int index) { item_consume(KIND_APPLE, index); }

int pear_count(void) { return item_count(KIND_PEAR); }
bool pear_alive(int index) { return item_alive(KIND_PEAR, index); }
uint16_t pear_cell(int index) { return item_cell(KIND_PEAR, index); }
int pear_index_at(uint16_t cell) { return item_index_at(KIND_PEAR, cell); }
void pear_consume(int index) { item_consume(KIND_PEAR, index); }

/* ------------------------------------------------------------------ */
/* View                                                                */
/* ------------------------------------------------------------------ */

void items_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor white = view_rgb(255, 255, 255);
    ViewColor black = view_rgb(0, 0, 0);
    int apple_frame = (int)view_sprite_clock(LONGO_SPR_APPLE) % 8;
    int pear_frame = (int)view_sprite_clock(LONGO_SPR_PEAR) % 8;
    for (int kind = 0; kind < KIND_COUNT; kind++) {
        LongoSprite spr = kind == KIND_APPLE ? LONGO_SPR_APPLE : LONGO_SPR_PEAR;
        int frame = kind == KIND_APPLE ? apple_frame : pear_frame;
        for (int i = 0; i < item_cnt[kind]; i++) {
            if (!items[kind][i].alive) continue;
            float x = (float)(sim_cell_x(items[kind][i].cell) * 16);
            float y = (float)(sim_cell_y(items[kind][i].cell) * 16);
            if (shadow)
                view_sprite(0, spr, frame, x, y + 7.0f, 1.0f, 0.6f, 0.0f,
                            black, 1.0f);
            else
                view_sprite(VIEW_DEPTH_ITEM, spr, frame, x, y, 1.0f, 1.0f,
                            0.0f, white, 1.0f);
        }
    }
}
