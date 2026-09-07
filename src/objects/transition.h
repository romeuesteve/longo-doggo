/*
 * Transition object script: the level-wipe state machine (the one script
 * that keeps animated floats).  It persists across room loads.  One wipe
 * phase (idle / opening / closing) plus one pending room action; at the
 * midpoint of the opening wipe it applies the action through the world
 * (sim_room_restart() or sim_room_goto_next()).
 */
#ifndef LONGO_OBJECT_TRANSITION_H
#define LONGO_OBJECT_TRANSITION_H

#include <stdbool.h>

#include "../core/world.h"

typedef enum TransitionPhase {
    TRANSITION_IDLE = 0, /* no wipe on screen */
    TRANSITION_OPENING,  /* wipe covers the screen; reload waits inside */
    TRANSITION_CLOSING   /* wipe uncovers; the room change already happened */
} TransitionPhase;

typedef enum TransitionAction {
    TRANSITION_ACTION_NONE = 0,
    TRANSITION_ACTION_RETRY,    /* midpoint reloads the current room */
    TRANSITION_ACTION_NEXT_ROOM /* midpoint advances to the next room */
} TransitionAction;

typedef struct TransitionState {
    TransitionPhase phase;
    TransitionAction pending; /* applied at the opening wipe's midpoint */
    float x, text_y;
    int room_num; /* win counter; labels levels ("LEVEL 1", "LEVEL 2", ...) */
} TransitionState;

void transition_reset(void);
void transition_tick(SimWorld *w); /* may reload the room mid-tick */

const TransitionState *transition_state(void);
bool transition_closing(void); /* true while the closing wipe runs; gates
                                  retry (sim_tick) and dialogue advance */

void transition_request_retry(void);
void transition_request_next(void);
void transition_count_win(void); /* room_num++ (label counts wins) */

/* View. */
void transition_draw(void); /* GUI layer */

#endif /* LONGO_OBJECT_TRANSITION_H */
