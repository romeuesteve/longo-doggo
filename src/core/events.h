/*
 * Event queues: the one channel from object scripts to the platform
 * (sounds) and the view (visual effects).  Scripts emit; main/render poll.
 */
#ifndef LONGO_EVENTS_H
#define LONGO_EVENTS_H

#include <stdint.h>

#define EVENTS_MAX_SOUNDS 64
#define EVENTS_MAX_FX 64

/* Sound asset indices recovered from data.win. */
typedef enum GameSound {
    SND_NONE = -1,
    SND_PLACEHOLDER = 0,
    SND_POOF = 1,
    SND_WRONG = 2,
    SND_PUSHED = 3,
    SND_WIN = 4,
    SND_BUTTON = 5,
    SND_BARK = 6
} GameSound;

typedef enum FxKind {
    FX_NONE = 0,
    FX_SMOKE_BURST, /* count puffs scattered around (x, y) */
    FX_BARK,        /* bark wedge at (x, y) rotated angle */
    FX_ONE,         /* "1" popup at (x, y); variant 1 = shrinking popup */
    FX_BOX_SINK     /* box visual gliding from (x, y) into the hole at
                     * (tx, ty), then vanishing */
} FxKind;

typedef struct FxEvent {
    FxKind kind;
    float x, y;
    float tx, ty; /* box sink target */
    float angle;  /* bark */
    int count;    /* smoke burst puff count */
    int variant;  /* one popup sprite index */
} FxEvent;

typedef struct SoundEvent {
    int sound;
    int loop;
} SoundEvent;

/* Clears both queues; world_tick calls this at the start of every tick. */
void events_clear(void);

void events_sound(int sound, int loop);
void events_fx(FxKind kind, float x, float y, float tx, float ty, float angle,
               int count, int variant);

int events_poll_sounds(SoundEvent *out);
int events_poll_fx(FxEvent *out);

#endif /* LONGO_EVENTS_H */
