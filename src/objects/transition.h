/*
 * Transition object script: the level-wipe state machine (ported as-is;
 * it is the one script that keeps animated floats).  It persists across
 * room loads like the original persistent instance.  At the wipe midpoint
 * it applies the pending room change through the world.
 */
#ifndef LONGO_OBJECT_TRANSITION_H
#define LONGO_OBJECT_TRANSITION_H

#include <stdbool.h>

typedef struct TransitionState {
    bool active;
    bool open, close, retry, next_lvl;
    float x, text_y;
    int room_num; /* win counter; labels levels ("LEVEL 1", "LEVEL 2", ...) */
} TransitionState;

void transition_reset(void);
void transition_tick(void); /* may reload the room mid-tick */

const TransitionState *transition_state(void);
bool transition_closing(void); /* gates dialogue advance and retry */

void transition_request_retry(void);
void transition_request_next(void);
void transition_count_win(void); /* room_num++ (label counts wins) */

/* View. */
void transition_draw(void); /* GUI layer */

#endif /* LONGO_OBJECT_TRANSITION_H */
