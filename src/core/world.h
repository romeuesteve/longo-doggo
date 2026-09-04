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

/* Fixed movement repeat: the original derived its held-key cadence from a
 * 2-tick alarm plus the OS key-repeat rate, which lands on one cell every
 * 2 ticks (30 cells/s) once a key is held.  The sim now owns the cadence
 * explicitly; a fresh press still steps immediately. */
#define SIM_STEP_INTERVAL 2

/* Dialogue box shrink after the final press (lerp 0.15 below 0.65 scale). */
#define SIM_DIALOGUE_SHRINK_TICKS 34

#define SIM_MAX_CHAIN 64 /* dog body parts (original LONGO_MAX_DOG_INS) */
#define SIM_MAX_BUTTONS 16
#define SIM_MAX_DOORS 8
#define SIM_MAX_ZONE 16  /* cells covered by one placed object bbox */

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

typedef struct SimDog {
    int alive;
    int cx, cy;  /* head cell */
    int dir;     /* 0 down, 90 right, 180 up, 270 left (GameMaker degrees) */
    int play;    /* dialogue gating */
    int length;  /* number of body parts */
    uint16_t chain[SIM_MAX_CHAIN];  /* chain[0] is nearest the head */
    uint8_t pflag[SIM_MAX_CHAIN];
    int strain;       /* blocked on the last attempted step (logical only) */
    int move_timer;   /* ticks until the next held-repeat step */
    int bark_timer;   /* idle bark alarm, -1 = disabled (title only) */
    int detached_cell; /* cell the tail vacated on the last step */
} SimDog;

/* Buttons cover one or two cells: the original places them straddling a
 * cell boundary, and anything overlapping the button rect pressed it.
 * zone = cells a 16x16 body (head/parts/holes/doors) presses;
 * box_zone additionally includes the straddle cell a box presses through
 * its 4px sprite lid. */
typedef struct SimButton {
    int alive;
    int pressed;
    uint16_t zone[SIM_MAX_ZONE];
    int zone_count;
    uint16_t box_zone[SIM_MAX_ZONE];
    int box_zone_count;
} SimButton;

typedef struct SimDoor {
    int alive;
    int open;        /* all buttons pressed; cell stays solid until removed */
    int open_timer;  /* the open animation ticks, then the door poofs */
    uint16_t cell;
} SimDoor;

/* oTutorial dialogue state machine (texts ported verbatim). */
typedef struct SimDbox {
    float x, y;
    const char *text;
} SimDbox;

typedef struct SimDialogue {
    int active;
    int index;         /* current box (i) */
    int last;          /* last box index (num) */
    int release_ticks; /* ticks since the final press (-1 = not released) */
    int gates_play;    /* this room's dialogue pauses the dog */
    float base_scale;  /* dialogue box pop-in target scale (4, or 10 credits) */
    SimDbox box[8];
} SimDialogue;

/* oTransition: the one state machine the sim keeps animated fields for,
 * ported as-is; the renderer draws straight from it.  It persists across
 * room loads, exactly like the original persistent instance. */
typedef struct SimTransition {
    int active;
    int open_transition, close_transition, retry, next_lvl;
    float x, text_y;
    int room_num; /* win counter; labels levels ("LEVEL 1", "LEVEL 2", ...) */
} SimTransition;

typedef struct SimWorld {
    int room_index;
    const LongoRoom *room;
    int cells_w, cells_h;
    long tick;
    long room_loaded_tick; /* guards same-tick meta steps after a reload */

    /* static solids: oBlock cells plus the goal house footprint */
    uint8_t solid[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];

    SimDog dog;
    SimButton buttons[SIM_MAX_BUTTONS];
    int button_count;
    int buttons_pressed;
    SimDoor doors[SIM_MAX_DOORS];
    int door_count;

    int has_title;
    SimDialogue dialogue;
    SimTransition trans;

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

/* Cell helpers shared by the rules. */
int sim_cell_x(uint16_t cell);
int sim_cell_y(uint16_t cell);
uint16_t sim_cell_of(int cx, int cy);

/* Entity-at-cell lookups (used by the rules and the presentation). */
int sim_part_at(const SimWorld *w, uint16_t cell);

/* Random in [0, max) / [lo, hi) from the sim's xorshift (cosmetic scatter). */
float sim_random(SimWorld *w, float max);
float sim_random_range(SimWorld *w, float lo, float hi);

/* Dog movement, chain and pickup rules (sim_dog.c). */
void sim_dog_step(SimWorld *w, const SimInput *input);

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
