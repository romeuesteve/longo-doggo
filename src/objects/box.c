#include "box.h"

#include <string.h>

#include "../core/events.h"
#include "../core/view.h"
#include "../core/solid.h"
#include "../core/world.h"
#include "../objects/dog.h"
#include "hole.h"

static Box boxes[BOX_MAX];
static int box_cnt;

static void view_snap(int index); /* placement starts the eased position */

/* The box owns its one-cell footprint: every placement, move or removal
 * goes through this pair, so a live box is stamped exactly on its cell
 * and a dead one holds no cell at all. */
static void box_stamp(int index)
{
    solid_place(boxes[index].cell, SOLID_BOX, index);
}

static void box_unstamp(int index)
{
    solid_clear_owned(boxes[index].cell, SOLID_BOX, index);
}

void box_reset(void)
{
    memset(boxes, 0, sizeof(boxes));
    box_cnt = 0;
}

void box_capture(BoxSnapshot *out)
{
    memcpy(out->boxes, boxes, sizeof(boxes));
    out->count = box_cnt;
}

void box_restore(const BoxSnapshot *snap)
{
    memcpy(boxes, snap->boxes, sizeof(boxes));
    box_cnt = snap->count;
    for (int i = 0; i < box_cnt; i++) view_snap(i);
}

void box_place(uint16_t cell)
{
    if (box_cnt >= BOX_MAX) return;
    boxes[box_cnt].alive = true;
    boxes[box_cnt].cell = cell;
    box_stamp(box_cnt);
    view_snap(box_cnt);
    box_cnt++;
}

void box_set_cell(int index, uint16_t cell)
{
    if (index < 0 || index >= box_cnt) return;
    if (!boxes[index].alive) {
        boxes[index].cell = cell;
        return;
    }
    box_unstamp(index);
    boxes[index].cell = cell;
    box_stamp(index);
}

int box_count(void) { return box_cnt; }

bool box_alive(int index)
{
    return index >= 0 && index < box_cnt && boxes[index].alive;
}

uint16_t box_cell(int index) { return boxes[index].cell; }

int box_index_at(uint16_t cell)
{
    for (int i = 0; i < box_cnt; i++)
        if (boxes[i].alive && boxes[i].cell == cell) return i;
    return -1;
}

bool box_push(int index, uint16_t from_cell, int dir)
{
    if (!box_alive(index)) return false;

    uint16_t beyond = cell_neighbour(from_cell, dir);
    int bcx = sim_cell_x(beyond);
    int bcy = sim_cell_y(beyond);
    if (!cell_in_bounds(bcx, bcy)) return false;

    int hole = hole_index_at(beyond);
    if (hole >= 0 && !hole_is_full(hole)) {
        /* an open hole swallows the box and counts as filled; a filled
         * hole is normal ground and the box lands on top of it */
        box_unstamp(index);
        boxes[index].alive = false;
        hole_fill(hole);
        events_sound(SND_POOF, 0);
        events_fx(FX_BOX_SINK, (float)(sim_cell_x(from_cell) * SIM_CELL),
                  (float)(sim_cell_y(from_cell) * SIM_CELL),
                  (float)(bcx * SIM_CELL), (float)(bcy * SIM_CELL), 0, 0, 0);
        return true;
    }

    if (solid_blocks_box(beyond)) return false;

    box_unstamp(index);
    boxes[index].cell = beyond;
    box_stamp(index);
    events_sound(SND_PUSHED, 0);
    return true;
}


/* ------------------------------------------------------------------ */
/* View: eased position (0.25 lerp per tick)                           */
/* ------------------------------------------------------------------ */

static float v_box_x[BOX_MAX], v_box_y[BOX_MAX];

/* A fresh box sits on its cell; only pushed boxes ease, starting from
 * the placed position, never from the origin. */
static void view_snap(int index)
{
    v_box_x[index] = (float)(sim_cell_x(boxes[index].cell) * SIM_CELL);
    v_box_y[index] = (float)(sim_cell_y(boxes[index].cell) * SIM_CELL);
}

float box_visual_x(int index) { return v_box_x[index]; }
float box_visual_y(int index) { return v_box_y[index]; }

void box_view_tick(void)
{
    for (int i = 0; i < box_cnt; i++) {
        if (!boxes[i].alive) continue;
        float tx = (float)(sim_cell_x(boxes[i].cell) * SIM_CELL);
        float ty = (float)(sim_cell_y(boxes[i].cell) * SIM_CELL);
        v_box_x[i] += (tx - v_box_x[i]) * 0.25f;
        v_box_y[i] += (ty - v_box_y[i]) * 0.25f;
    }
}

void box_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor tint = shadow ? view_rgb(0, 0, 0) : view_rgb(255, 255, 255);
    for (int i = 0; i < box_cnt; i++) {
        if (!boxes[i].alive) continue;
        if (shadow) {
            /* the shadow layer is composited flat; every item is depth 0 */
            view_sprite(0, LONGO_SPR_BOX, 0, v_box_x[i], v_box_y[i] + 5.0f,
                        1.0f, 1.0f, 0.0f, tint, 1.0f);
        } else {
            /* dynamic depth: -100 - y / 6 */
            view_sprite((int)(VIEW_DEPTH_BOX_BASE - v_box_y[i] / 6),
                        LONGO_SPR_BOX, 0, v_box_x[i], v_box_y[i], 1.0f, 1.0f,
                        0.0f, tint, 1.0f);
        }
    }
}
