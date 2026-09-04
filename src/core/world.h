/*
 * Longo Doggo simulation core.
 *
 * Pure C, no rendering dependency, integer cell state.  Every gameplay
 * position is a 16px grid cell; movement snaps instantly and the
 * presentation layer (src/pres.c) eases visual positions toward the
 * snapped cells.  All rules are ports of the recovered GML behaviours,
 * translated from bbox probes to cell lookups.
 *
 * One sim_tick() is one 60 Hz frame, in the original event order:
 * dog step -> pickups -> buttons/doors/goal -> title/dialogue ->
 * transition (which may reload the room mid-tick).
 */
#ifndef LONGO_SIM_H
#define LONGO_SIM_H

#include <stdbool.h>
#include <stdint.h>

#include "../level_data.h"
#include "../sprites.h"
#include "events.h"

#define SIM_CELL 16
#define SIM_MAX_CELLS_W 24
#define SIM_MAX_CELLS_H 16
#define SIM_CELL_INDEX(cx, cy) ((uint16_t)((cy)*SIM_MAX_CELLS_W + (cx)))


typedef struct SimInput {
    /* held directions (arrow keys and WASD merged by the front-end) */
    unsigned char held_right, held_left, held_up, held_down;
    /* pressed edges, one tick wide */
    unsigned char pressed_space, pressed_enter, pressed_e, pressed_r;
    unsigned char pressed_any;
} SimInput;

/* Per-part flags (parallel to SimDog.chain).
 * first: part directly behind the head (front legs).
 * legs:  draws walking legs.
 * butt:  solid for the dog's own movement probe (the chain body);
        the last part is not solid so the dog can follow onto it. */
#define SIM_PART_FIRST 0x1
#define SIM_PART_LEGS 0x2
#define SIM_PART_BUTT 0x4

typedef struct SimWorld {
    SimInput input; /* last fed input, readable by scripts via world_ptr() */
    int room_index;
    const LongoRoom *room;
    int cells_w, cells_h;
    long tick;
    long room_loaded_tick; /* guards same-tick meta steps after a reload */


    unsigned int rng;
} SimWorld;

SimWorld *world_ptr(void);
void sim_init(unsigned int seed);
void sim_room_goto(SimWorld *w, int room_index);
void sim_room_goto_next(SimWorld *w);
void sim_room_restart(SimWorld *w);

void sim_tick(SimWorld *w, const SimInput *input);

bool cell_in_bounds(int cx, int cy);
uint16_t cell_neighbour(uint16_t cell, int dir);
float world_random(float max);
float world_random_range(float lo, float hi);

/* Cell helpers shared by the rules. */
int sim_cell_x(uint16_t cell);
int sim_cell_y(uint16_t cell);
uint16_t sim_cell_of(int cx, int cy);

/* Entity-at-cell lookups (used by the rules and the presentation). */

/* Random in [0, max) / [lo, hi) from the sim's xorshift (cosmetic scatter). */
float sim_random(SimWorld *w, float max);
float sim_random_range(SimWorld *w, float lo, float hi);


/* Room order indices into longo_rooms[] (GeneralInfo.RoomOrder). */
enum {
    SIM_ROOM_TITLE = 0,
    SIM_ROOM_TUTORIAL = 1,
    SIM_ROOM_LEVEL6 = 2,
    SIM_ROOM_LEVEL5 = 3,
    SIM_ROOM_LEVEL4 = 4,
    SIM_ROOM_LEVEL2 = 5,
    SIM_ROOM_LEVEL1 = 6,
    SIM_ROOM_LEVEL3 = 7,
    SIM_ROOM_CREDITS = 8,
    SIM_ROOM_COUNT = 9 /* the editor and levelbase rooms are dropped */
};

#endif /* LONGO_SIM_H */
