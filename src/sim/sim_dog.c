/*
 * Dog movement rules: fully tile-based.
 *
 * The head occupies a cell; a step snaps it into the adjacent cell
 * instantly and the body chain shifts one cell along (classic snake).
 * The presentation eases sprite positions toward these cells, which is
 * what keeps the on-screen motion smooth.
 *
 * Rule ports from the recovered GML:
 *   - MoveDogX/MoveDogY: instant 16px logical step, facing/sprite update
 *   - the +10/+8 bbox probes: cell lookups (wall / box / own body)
 *   - the tail exception (a part with block=0): the last chain cell does
 *     not block the head or a pushed box, because it vacates in the same
 *     tick
 *   - oApple/oSkull/oWin collision events: pickups on the head's new cell
 */
#include "sim.h"

#include <math.h>

static int in_bounds(const SimWorld *w, int cx, int cy)
{
    return cx >= 0 && cy >= 0 && cx < w->cells_w && cy < w->cells_h;
}

static SimDoor *door_at(SimWorld *w, uint16_t cell)
{
    for (int i = 0; i < w->door_count; i++)
        if (w->doors[i].alive && w->doors[i].cell == cell) return &w->doors[i];
    return NULL;
}

static uint16_t head_cell(const SimWorld *w)
{
    return sim_cell_of(w->dog.cx, w->dog.cy);
}

static uint16_t neighbour(uint16_t cell, int dir)
{
    int cx = sim_cell_x(cell);
    int cy = sim_cell_y(cell);
    switch (dir) {
    case 0: return sim_cell_of(cx, cy + 1);   /* down */
    case 90: return sim_cell_of(cx + 1, cy);  /* right */
    case 180: return sim_cell_of(cx, cy - 1); /* up */
    default: return sim_cell_of(cx - 1, cy);  /* left */
    }
}

static void shift_chain(SimDog *dog, uint16_t old_head)
{
    /* The cell the tail vacates is where a grown segment reappears. */
    dog->detached_cell = dog->chain[dog->length - 1];
    for (int i = dog->length - 1; i > 0; i--)
        dog->chain[i] = dog->chain[i - 1];
    dog->chain[0] = old_head;
}

/* The cell content checks, in the original probe order: static walls,
 * open holes, doors (solid until their open animation poofs them), the
 * dog's own body (butt parts only; the tail is followable). */
static int blocked_for(SimWorld *w, uint16_t cell)
{
    if (w->solid[cell]) return 1;
    if (sim_hole_at(w, cell)) return 1;
    if (door_at(w, cell)) return 1;
    int part = sim_part_at(w, cell);
    if (part >= 0 && (w->dog.pflag[part] & SIM_PART_BUTT)) return 1;
    return 0;
}

static void grow_chain(SimWorld *w, uint16_t at_cell)
{
    SimDog *dog = &w->dog;
    if (dog->length >= SIM_MAX_CHAIN) return;
    /* the former tail becomes plain body; the new tail gets legs */
    dog->pflag[dog->length - 1] = SIM_PART_BUTT;
    dog->chain[dog->length] = at_cell;
    dog->pflag[dog->length] = SIM_PART_LEGS;
    dog->length++;
}

static void shrink_chain(SimWorld *w)
{
    SimDog *dog = &w->dog;
    dog->length--;
    dog->pflag[dog->length - 1] = SIM_PART_LEGS;
}

/* Pickups on the head's new cell (oApple/oSkull/oWin collision events). */
static void resolve_pickups(SimWorld *w)
{
    SimDog *dog = &w->dog;
    uint16_t head = head_cell(w);
    float hx = (float)(sim_cell_x(head) * SIM_CELL + 8);
    float hy = (float)(sim_cell_y(head) * SIM_CELL + 8);

    for (int i = 0; i < w->apple_count; i++) {
        SimItem *apple = &w->apples[i];
        if (!apple->alive || apple->cell != head) continue;
        apple->alive = 0;
        grow_chain(w, dog->detached_cell);
        sim_emit_fx(w, SIM_FX_ONE, hx, hy - 8.0f, 0, 0, 0);
        sim_emit_fx(w, SIM_FX_SMOKE_BURST, hx, hy, 0, 7, 0);
        sim_play_sound(w, LONGO_SND_POOF, 0);
    }

    for (int i = 0; i < w->skull_count; i++) {
        SimItem *skull = &w->skulls[i];
        if (!skull->alive || skull->cell != head) continue;
        skull->alive = 0;
        if (dog->length > 2) {
            uint16_t tail = dog->chain[dog->length - 1];
            shrink_chain(w);
            sim_emit_fx(w, SIM_FX_SMOKE_BURST,
                        (float)(sim_cell_x(tail) * SIM_CELL + 8),
                        (float)(sim_cell_y(tail) * SIM_CELL + 8), 0, 7, 0);
            sim_emit_fx(w, SIM_FX_ONE, hx, hy - 8.0f, 0, 0, 1);
        } else {
            dog->alive = 0;
        }
        sim_play_sound(w, LONGO_SND_POOF, 0);
    }

    if (dog->alive && w->win.alive && w->goal.alive && w->goal.remain <= 0) {
        for (int z = 0; z < w->win.zone_count; z++) {
            if (w->win.zone[z] == head) {
                w->win.alive = 0;
                w->trans.active = 1;
                w->trans.room_num++;
                w->trans.next_lvl = 1;
                w->trans.open_transition = 1;
                break;
            }
        }
    }
}

/* Attempt one cell step; returns 1 on success.  A box in the target cell
 * is pushed into the next cell when that cell is free, an open hole (the
 * box fills it), or the dog's own tail (which vacates this tick). */
static int try_step(SimWorld *w, int dir)
{
    SimDog *dog = &w->dog;
    uint16_t target = neighbour(head_cell(w), dir);
    int tcx = sim_cell_x(target);
    int tcy = sim_cell_y(target);

    if (!in_bounds(w, tcx, tcy)) return 0;
    if (blocked_for(w, target)) {
        dog->strain = 1;
        return 0;
    }

    SimBox *box = sim_box_at(w, target);
    if (box) {
        uint16_t beyond = neighbour(target, dir);
        int bcx = sim_cell_x(beyond);
        int bcy = sim_cell_y(beyond);
        SimHole *hole = in_bounds(w, bcx, bcy) ? sim_hole_at(w, beyond) : NULL;
        if (hole) {
            box->alive = 0;
            hole->full = 1;
            {
                /* the box visual glides into the hole, then vanishes */
                SimFx *fx = &w->fx[w->fx_count < SIM_MAX_FX ? w->fx_count : 0];
                if (w->fx_count < SIM_MAX_FX) {
                    fx = &w->fx[w->fx_count++];
                    fx->kind = SIM_FX_BOX_SINK;
                    fx->x = (float)(tcx * SIM_CELL);
                    fx->y = (float)(tcy * SIM_CELL);
                    fx->tx = (float)(bcx * SIM_CELL);
                    fx->ty = (float)(bcy * SIM_CELL);
                    fx->angle = 0;
                    fx->count = 0;
                    fx->variant = 0;
                }
            }
            sim_play_sound(w, LONGO_SND_POOF, 0);
        } else if (in_bounds(w, bcx, bcy) && !blocked_for(w, beyond) &&
                   !sim_box_at(w, beyond)) {
            box->cell = beyond;
            sim_play_sound(w, LONGO_SND_PUSHED, 0);
        } else {
            dog->strain = 1;
            return 0;
        }
    }

    int old_cx = dog->cx;
    int old_cy = dog->cy;
    dog->cx = tcx;
    dog->cy = tcy;
    dog->dir = dir;
    dog->strain = 0;
    shift_chain(dog, sim_cell_of(old_cx, old_cy));
    return 1;
}

static void emit_bark(SimWorld *w)
{
    SimDog *dog = &w->dog;
    float hx = (float)(sim_cell_x(head_cell(w)) * SIM_CELL + 8);
    float hy = (float)(sim_cell_y(head_cell(w)) * SIM_CELL + 8);
    float fx = hx, fy = hy;
    switch (dog->dir) {
    case 0: fy += 12.0f; break;
    case 90: fx += 12.0f; break;
    case 180: fy -= 12.0f; break;
    default: fx -= 12.0f; break;
    }
    sim_emit_fx(w, SIM_FX_BARK, fx, fy, (float)dog->dir + 180.0f, 0, 0);
    sim_play_sound(w, LONGO_SND_BARK, 0);
}

static int sign(int v)
{
    return (v > 0) - (v < 0);
}

void sim_dog_step(SimWorld *w, const SimInput *input)
{
    SimDog *dog = &w->dog;
    if (!dog->alive) return;

    /* oDog Step: R retries unless a wipe is closing */
    if (input->pressed_r && !w->trans.close_transition) {
        w->trans.active = 1;
        w->trans.retry = 1;
        w->trans.open_transition = 1;
    }

    /* the title screen parks the dog */
    if (w->has_title) dog->play = 0;

    if (dog->play && input->pressed_space) emit_bark(w);

    if (dog->move_timer > 0) dog->move_timer--;

    int xm = sign((input->held_right ? 1 : 0) - (input->held_left ? 1 : 0));
    int ym = sign((input->held_down ? 1 : 0) - (input->held_up ? 1 : 0));

    /* horizontal wins diagonal input, like the original's xmove-first check */
    if (dog->play && dog->move_timer == 0 && (xm != 0 || ym != 0)) {
        int moved = 0;
        if (xm != 0)
            moved = try_step(w, xm > 0 ? 90 : 270);
        else
            moved = try_step(w, ym > 0 ? 0 : 180);
        if (moved) dog->move_timer = SIM_STEP_INTERVAL;
    }

    /* idle bark (armed only by the title room, like the oDog alarm) */
    if (dog->bark_timer > 0) {
        if (--dog->bark_timer == 0) {
            if (sim_random(w, 1.0f) < 0.2f) emit_bark(w);
            dog->bark_timer = 25;
        }
    }

    resolve_pickups(w);
}
