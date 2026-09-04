/*
 * Presentation state: everything floaty and visual.
 *
 * The simulation snaps positions to cells; this module eases sprite
 * positions toward those cells (the original xx/yy lerp, promoted to the
 * only interpolation path), advances animation clocks, runs cosmetic
 * particles (smoke, popups, barks, box sinks), the door/goal/dialogue
 * scale animations, the title/title waves, and ambient butterflies.
 *
 * pres_update() runs once per rendered frame (60 Hz vsynced, matching the
 * original tick-coupled easing constants).
 */
#ifndef LONGO_PRES_H
#define LONGO_PRES_H

#include "world.h"

#define PRES_MAX_FLOWERS 64
#define PRES_MAX_FLIES 8
#define PRES_MAX_SMOKE 160
#define PRES_MAX_POPUPS 16
#define PRES_MAX_BARKS 8
#define PRES_MAX_SINKS 8

typedef struct PresSmoke {
    int alive;
    float x, y;
    float dir;   /* motion direction, degrees */
    float speed;
    float angle; /* sprite rotation */
    float spin;  /* degrees per tick */
    float scale; /* image_xscale == image_yscale */
} PresSmoke;

typedef struct PresPopup {
    int alive;
    float x, y;
    float alpha;
    int variant; /* sprOne frame */
} PresPopup;

typedef struct PresBark {
    int alive;
    float x, y;
    float angle;
    float frame;
} PresBark;

typedef struct PresSink {
    int alive;
    float x, y;
    float tx, ty;
} PresSink;

typedef struct PresButterfly {
    int alive;
    float x, y;
    float xstart, ystart;
    float hspd, vspd;
    float dir;
    int timer;
} PresButterfly;

typedef struct PresDoor {
    float scale_x, scale_y; /* eased open squash */
    float x, y;             /* shifted draw position */
} PresDoor;

typedef struct Pres {
    const LongoRoom *room; /* decor scan cache; snaps when the room changes */
    unsigned int rng;
    float time_ms;

    /* eased positions (pixels; dog/parts are sprite centres, cell + 8) */
    float dog_x, dog_y;
    float part_x[SIM_MAX_CHAIN], part_y[SIM_MAX_CHAIN];
    uint16_t part_cell[SIM_MAX_CHAIN];
    int part_wiggle[SIM_MAX_CHAIN];
    float box_x[SIM_MAX_BOXES], box_y[SIM_MAX_BOXES];

    PresDoor doors[SIM_MAX_DOORS];

    /* goal house pulse (oGoalUp count/count2 port) */
    int goal_count, goal_count2;
    float goal_scale_x, goal_scale_y;

    /* dialogue box scale */
    float dlg_scale_x, dlg_scale_y;

    /* animation clocks (image_index += fps / 60 per frame) */
    float flower_clock;
    float apple_clock;
    float pear_clock;
    float fly_clock;
    float button_clock;
    float block_clock;

    PresSmoke smoke[PRES_MAX_SMOKE];
    int smoke_count;
    PresPopup popups[PRES_MAX_POPUPS];
    int popup_count;
    PresBark barks[PRES_MAX_BARKS];
    int bark_count;
    PresSink sinks[PRES_MAX_SINKS];
    int sink_count;

    PresButterfly flies[PRES_MAX_FLIES];
    struct { float x, y; } flowers[PRES_MAX_FLOWERS];
    int flower_count;
    int shadows_present;
    float title_x, title_y;
    int has_title_decor;
} Pres;

void pres_init(Pres *p, unsigned int seed);
/* Ease toward the sim state and run particles; consumes the sim fx queue. */
void pres_update(Pres *p, SimWorld *w, const SimInput *input);

/* Legs wiggle amplitude for a part: 30 while the wiggle timer runs. */
float pres_part_legs_angle(const Pres *p, int part);

#endif /* LONGO_PRES_H */
