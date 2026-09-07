/*
 * Longo Doggo simulation core.
 *
 * Pure C, no rendering dependency, integer cell state.  Every gameplay
 * position is a 16px grid cell; movement snaps instantly and the
 * presentation layer (core/view.c + render.c) eases visual positions
 * toward the snapped cells.  All rules are expressed as cell lookups
 * over the 16px grid.
 *
 * One sim_tick() is one 60 Hz frame, in this order:
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
    /* pressed edges, one tick wide.  Movement consumes one of these per
     * step: one step per physical press, not per held-key frame.  Held
     * state is not represented; consumers read edges only. */
    unsigned char pressed_right, pressed_left, pressed_up, pressed_down;
    unsigned char pressed_space, pressed_enter, pressed_e, pressed_r;
    unsigned char pressed_undo;
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
    int shadows_present;   /* the room has a shadow-caster object */


    unsigned int rng;
} SimWorld;

SimWorld *world_ptr(void);
/* NEW GAME: the one complete reset.  Reinitializes the SimWorld (rng
 * seed, tick = 1), zeroes the view module's animation clocks and loads
 * the title room.  Room loads and retries (sim_room_goto/restart) reset
 * room-scoped state only; see load_room in core/world.c for the list of
 * what deliberately survives them. */
void sim_init(unsigned int seed);
void sim_room_goto(SimWorld *w, int room_index);
void sim_room_goto_next(SimWorld *w);
void sim_room_restart(SimWorld *w);

/* One update is one 60 Hz step.  sim_frame never runs more than
 * SIM_MAX_CATCHUP updates per call: a longer stall drops the backlog and
 * resumes at the next update instead of fast-forwarding. */
#define SIM_STEP_MS (1000.0 / 60.0)
#define SIM_MAX_CATCHUP 5

/* Per-update sound delivery: the schedule calls it right after each
 * update's view pass, so the update's queued sounds reach the audio
 * device before the next update's events_clear().  May be NULL
 * (headless). */
typedef void (*SimSoundDeliver)(void *user);

/* The fixed-step schedule shared by the native loop, the web loop and
 * the tests: advances the game by dt_ms of real time in SIM_STEP_MS
 * updates, at most SIM_MAX_CATCHUP per call.  The input's pressed edges
 * are consumed by the first update that runs after they were sampled; a
 * call that runs zero updates holds them (catch-up updates run without
 * edges). */
void sim_frame(double dt_ms, const SimInput *input,
               SimSoundDeliver deliver_sounds, void *user);

/* One rules tick = one 1/60 s step.  It owns the 60 Hz schedule: it
 * steps the view animation clocks (view_update) and clears the event
 * queues at the top, so whatever a tick queues must be delivered before
 * the next one (the sim_frame schedule does exactly that). */
void sim_tick(SimWorld *w, const SimInput *input);

/* View orchestration: easing ticks then draw-item push, in one place. */
void world_view_tick(const SimInput *input);
void world_draw(void);

bool cell_in_bounds(int cx, int cy);
uint16_t cell_neighbour(uint16_t cell, int dir);

/* Cell helpers shared by the rules. */
int sim_cell_x(uint16_t cell);
int sim_cell_y(uint16_t cell);
uint16_t sim_cell_of(int cx, int cy);

/* Random in [0, max) / [lo, hi) from the sim's xorshift (the bark chance;
 * cosmetic view effects keep their own Rng streams, see core/rng.h). */
float world_random(float max);
float world_random_range(float lo, float hi);


/* Load-time validation of one catalog room (see longo_room_validate).
 * The report separates malformed content from intentional clips:
 * offscreen placements are authored decoration and only counted, while
 * dimensions that miss the sim grid and functional objects beyond an
 * entity pool's capacity are violations. */
typedef struct LongoRoomValidation {
    int violations;            /* malformed content, must be zero */
    int offscreen;             /* deliberate off-grid decoration count */
    char first_violation[192]; /* room name + cause, "" when clean */
} LongoRoomValidation;

/* Validate one room table against the sim grid and entity pool
 * capacities.  Works on any table — a catalog entry or a hand-built
 * one.  Returns true when clean; `out` (may be NULL) carries the counts
 * and the first violation's description. */
bool longo_room_data_validate(const LongoRoom *room, LongoRoomValidation *out);

/* Validate catalog room `index`: the table check above, plus the
 * catalog-level requirement that every in-play room owns its tile map
 * entry.  Returns true when clean; `out` (may be NULL). */
bool longo_room_validate(int room_index, LongoRoomValidation *out);

/* Named play indices into the room catalog (level_data.c).  The catalog
 * owns the order and the shipped-room count (longo_room_play_count());
 * these enumerators are readable names for the authored positions (the
 * tests and sim_room_goto callers use them).  Authored content that
 * needs a room's identity — the dialogue texts, the tile maps — keys off
 * the stable LongoRoomId the room carries, not off these positions. */
enum {
    SIM_ROOM_TITLE = 0,
    SIM_ROOM_TUTORIAL = 1,
    SIM_ROOM_LEVEL6 = 2,
    SIM_ROOM_LEVEL5 = 3,
    SIM_ROOM_LEVEL4 = 4,
    SIM_ROOM_LEVEL2 = 5,
    SIM_ROOM_LEVEL1 = 6,
    SIM_ROOM_LEVEL3 = 7,
    SIM_ROOM_CREDITS = 8
};

#endif /* LONGO_SIM_H */
