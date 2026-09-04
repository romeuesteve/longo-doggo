/*
 * Dog rules: fully tile-based.
 *
 * The head occupies a cell; a step snaps it into the adjacent cell
 * instantly and the body chain shifts one cell along (classic snake).
 * Movement rules are ports of the recovered GML behaviours translated
 * from bbox probes to cell lookups:
 *   - MoveDogX/MoveDogY: instant 16px logical step, facing update
 *   - the +10/+8 bbox probes: solid_probe() / box_push()
 *   - the tail exception: the last chain cell does not block, because it
 *     vacates in the same tick
 *   - oApple/oSkull/oWin collision events: pickups on the head's new cell
 */
#include "dog.h"

#include <math.h>
#include <string.h>

#include "../core/events.h"
#include "../core/solid.h"
#include "box.h"
#include "house.h"
#include "items.h"
#include "title.h"
#include "transition.h"

#define DOG_MAX_CHAIN 64
#define DOG_STEP_INTERVAL 2 /* one cell every 2 ticks held (30 cells/s) */

typedef struct Dog {
    bool alive;
    int cx, cy;  /* head cell */
    int dir;     /* 0 down, 90 right, 180 up, 270 left (GameMaker degrees) */
    bool play;   /* dialogue gating */
    int length;  /* number of body parts */
    uint16_t chain[DOG_MAX_CHAIN];
    uint8_t pflag[DOG_MAX_CHAIN];
    bool strain;      /* blocked on the last attempted step (logical only) */
    int move_timer;   /* ticks until the next held-repeat step */
    int bark_timer;   /* idle bark alarm, -1 = disabled (title only) */
    int detached_cell; /* cell the tail vacated on the last step */
} Dog;

static Dog dog;

static uint16_t head_cell(void) { return sim_cell_of(dog.cx, dog.cy); }

static int sign(int v) { return (v > 0) - (v < 0); }

/* ------------------------------------------------------------------ */
/* Placement                                                           */
/* ------------------------------------------------------------------ */

static void place_on_solid_map(void)
{
    solid_place(head_cell(), SOLID_HEAD, 0);
    for (int i = 0; i < dog.length; i++)
        solid_place(dog.chain[i], SOLID_BODY, i);
}

void dog_reset(void)
{
    memset(&dog, 0, sizeof(dog));
    dog.bark_timer = -1;
}

void dog_place(float x, float y)
{
    dog.alive = true;
    dog.cx = (int)floorf((x - 8.0f) / SIM_CELL);
    dog.cy = (int)floorf((y - 8.0f) / SIM_CELL);
    dog.dir = 180; /* facing up, chain trailing below */
    dog.play = true;
    dog.length = 5;
    dog.strain = false;
    dog.move_timer = 0;
    dog.bark_timer = -1;
    for (int i = 0; i < dog.length; i++) {
        dog.chain[i] = sim_cell_of(dog.cx, dog.cy + 1 + i);
        dog.pflag[i] = DOG_PART_BUTT;
        if (i == 0) dog.pflag[i] |= DOG_PART_FIRST | DOG_PART_LEGS;
        if (i == dog.length - 1) dog.pflag[i] = DOG_PART_LEGS;
    }
    dog.detached_cell = dog.chain[dog.length - 1];
    place_on_solid_map();
}

/* The title room rearranges the dog into an S-curve (port of the oTitle
 * create event) and arms its idle bark. */
void dog_title_arrangement(void)
{
    /* head first, then the five parts of the S-curve (pixel coords in the
     * oTitle create event, snapped to cells) */
    static const int cells[6][2] = {
        { 10, 10 }, { 10, 9 }, { 9, 9 }, { 8, 9 }, { 8, 10 }, { 9, 10 }
    };
    if (!dog.alive) return;
    dog.cx = cells[0][0];
    dog.cy = cells[0][1];
    dog.dir = 0;
    dog.bark_timer = 10;
    for (int i = 0; i < dog.length && i + 1 < 6; i++)
        dog.chain[i] = sim_cell_of(cells[i + 1][0], cells[i + 1][1]);
    place_on_solid_map();
}

/* ------------------------------------------------------------------ */
/* Accessors                                                           */
/* ------------------------------------------------------------------ */

bool dog_alive(void) { return dog.alive; }
bool dog_play(void) { return dog.play; }
void dog_set_play(bool play) { dog.play = play; }
int dog_cx(void) { return dog.cx; }
int dog_cy(void) { return dog.cy; }
int dog_dir(void) { return dog.dir; }
int dog_length(void) { return dog.length; }
uint16_t dog_part_cell(int part) { return dog.chain[part]; }
int dog_part_flags(int part) { return dog.pflag[part]; }
bool dog_part_is_solid(int part)
{
    return part >= 0 && part < dog.length && (dog.pflag[part] & DOG_PART_BUTT);
}
bool dog_strain(void) { return dog.strain; }
uint16_t dog_detached_cell(void) { return (uint16_t)dog.detached_cell; }

void dog_teleport(int cx, int cy)
{
    /* clear old cells, place the head on the new one; the chain follows
     * below like a fresh spawn (test hook) */
    solid_clear(head_cell());
    for (int i = 0; i < dog.length; i++) solid_clear(dog.chain[i]);
    dog.cx = cx;
    dog.cy = cy;
    dog.move_timer = 0;
    place_on_solid_map();
}

void dog_set_alive(bool alive)
{
    if (dog.alive == alive) return;
    if (dog.alive) {
        solid_clear(head_cell());
        for (int i = 0; i < dog.length; i++) solid_clear(dog.chain[i]);
        dog.alive = false;
    } else {
        dog.alive = true;
        place_on_solid_map();
    }
}

void dog_set_length(int length)
{
    /* test hook: grow/shrink the chain in place around the current tail */
    if (length < 0 || length > DOG_MAX_CHAIN) return;
    while (dog.length > length) {
        solid_clear(dog.chain[dog.length - 1]);
        dog.length--;
        if (dog.length > 0) dog.pflag[dog.length - 1] = DOG_PART_LEGS;
    }
    while (dog.length < length) {
        dog.chain[dog.length] = dog.chain[dog.length - 1];
        dog.pflag[dog.length] = DOG_PART_LEGS;
        dog.length++;
    }
    place_on_solid_map();
}

/* ------------------------------------------------------------------ */
/* Movement                                                            */
/* ------------------------------------------------------------------ */

static void shift_chain(uint16_t old_head)
{
    /* The cell the tail vacates is where a grown segment reappears. */
    solid_clear(dog.chain[dog.length - 1]);
    dog.detached_cell = dog.chain[dog.length - 1];
    for (int i = dog.length - 1; i > 0; i--) dog.chain[i] = dog.chain[i - 1];
    dog.chain[0] = old_head;
    solid_place(old_head, SOLID_BODY, 0);
}

static void grow_chain(uint16_t at_cell)
{
    if (dog.length >= DOG_MAX_CHAIN) return;
    /* the former tail becomes plain body; the new tail gets legs */
    dog.pflag[dog.length - 1] = DOG_PART_BUTT;
    dog.chain[dog.length] = at_cell;
    dog.pflag[dog.length] = DOG_PART_LEGS;
    dog.length++;
}

static void shrink_chain(void)
{
    solid_clear(dog.chain[dog.length - 1]);
    dog.length--;
    dog.pflag[dog.length - 1] = DOG_PART_LEGS;
}

static void emit_bark(void)
{
    float hx = (float)(sim_cell_x(head_cell()) * SIM_CELL + 8);
    float hy = (float)(sim_cell_y(head_cell()) * SIM_CELL + 8);
    float fx = hx, fy = hy;
    switch (dog.dir) {
    case 0: fy += 12.0f; break;
    case 90: fx += 12.0f; break;
    case 180: fy -= 12.0f; break;
    default: fx -= 12.0f; break;
    }
    events_fx(FX_BARK, fx, fy, 0, 0, (float)dog.dir + 180.0f, 0, 0);
    events_sound(SND_BARK, 0);
}

/* Pickups on the head's new cell (oApple/oSkull/oWin collision events). */
static void resolve_pickups(void)
{
    uint16_t head = head_cell();
    float hx = (float)(sim_cell_x(head) * SIM_CELL + 8);
    float hy = (float)(sim_cell_y(head) * SIM_CELL + 8);

    int apple = apple_index_at(head);
    if (apple >= 0) {
        apple_consume(apple);
        grow_chain(dog.detached_cell);
        events_fx(FX_ONE, hx, hy - 8.0f, 0, 0, 0, 0, 0);
        events_fx(FX_SMOKE_BURST, hx, hy, 0, 0, 0, 7, 0);
        events_sound(SND_POOF, 0);
    }

    int skull = skull_index_at(head);
    if (skull >= 0) {
        skull_consume(skull);
        if (dog.length > 2) {
            uint16_t tail = dog.chain[dog.length - 1];
            shrink_chain();
            events_fx(FX_SMOKE_BURST,
                      (float)(sim_cell_x(tail) * SIM_CELL + 8),
                      (float)(sim_cell_y(tail) * SIM_CELL + 8), 0, 0, 0, 7, 0);
            events_fx(FX_ONE, hx, hy - 8.0f, 0, 0, 0, 0, 1);
        } else {
            /* eating a pear at minimum length destroys the dog (original
             * behaviour; the room softlocks until retry) */
            dog.alive = false;
            solid_clear(head);
        }
        events_sound(SND_POOF, 0);
    }

    if (dog.alive && house_try_win(head)) {
        transition_count_win();
        transition_request_next();
    }
}

/* Attempt one cell step; returns true on success. */
static bool try_step(int dir)
{
    uint16_t target = cell_neighbour(head_cell(), dir);
    int tcx = sim_cell_x(target);
    int tcy = sim_cell_y(target);

    if (!cell_in_bounds(tcx, tcy)) return false;

    switch (solid_probe(target)) {
    case SOLID_PROBE_SOLID:
        dog.strain = true;
        return false;
    case SOLID_PROBE_BOX:
        if (!box_push(solid_index_at(target), target, dir)) {
            dog.strain = true;
            return false;
        }
        break;
    default:
        break;
    }

    uint16_t old_head = head_cell();
    dog.cx = tcx;
    dog.cy = tcy;
    dog.dir = dir;
    dog.strain = false;
    solid_place(target, SOLID_HEAD, 0);
    shift_chain(old_head);
    return true;
}

void dog_tick(const SimInput *input)
{
    if (!dog.alive) return;

    /* oDog Step: R retries unless a wipe is closing */
    if (input->pressed_r && !transition_closing()) transition_request_retry();

    /* the title screen parks the dog */
    if (title_present()) dog.play = false;

    if (dog.play && input->pressed_space) emit_bark();

    if (dog.move_timer > 0) dog.move_timer--;

    int xm = sign((input->held_right ? 1 : 0) - (input->held_left ? 1 : 0));
    int ym = sign((input->held_down ? 1 : 0) - (input->held_up ? 1 : 0));

    /* horizontal wins diagonal input, like the original's xmove-first check */
    if (dog.play && dog.move_timer == 0 && (xm != 0 || ym != 0)) {
        bool moved = xm != 0 ? try_step(xm > 0 ? 90 : 270)
                             : try_step(ym > 0 ? 0 : 180);
        if (moved) dog.move_timer = DOG_STEP_INTERVAL;
    }

    /* idle bark (armed only by the title room, like the oDog alarm) */
    if (dog.bark_timer > 0) {
        if (--dog.bark_timer == 0) {
            if (world_random(1.0f) < 0.2f) emit_bark();
            dog.bark_timer = 25;
        }
    }

    resolve_pickups();
}
