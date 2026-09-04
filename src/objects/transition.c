#include "transition.h"

#include "../core/world.h"

static TransitionState tr;

void transition_reset(void)
{
    /* oTransition create values */
    tr.active = true;
    tr.open = false;
    tr.close = false;
    tr.retry = false;
    tr.next_lvl = false;
    tr.x = 304.0f;
    tr.text_y = -16.0f;
    tr.room_num = 1;
}

const TransitionState *transition_state(void) { return &tr; }

bool transition_closing(void) { return tr.close; }

void transition_request_retry(void)
{
    tr.active = true;
    tr.retry = true;
    tr.open = true;
}

void transition_request_next(void)
{
    tr.active = true;
    tr.next_lvl = true;
    tr.open = true;
}

void transition_count_win(void) { tr.room_num++; }

/* oTransition Draw event port, verbatim. */
void transition_tick(void)
{
    if (!tr.active) return;
    if (tr.open) {
        if (tr.x >= -20.0f) {
            tr.x += (-31.0f - tr.x) * 0.04f;
        } else {
            tr.x = 304.0f;
            tr.close = true;
            if (tr.retry) {
                sim_room_restart(world_ptr());
                tr.retry = false;
            } else if (tr.next_lvl) {
                sim_room_goto_next(world_ptr());
                tr.next_lvl = false;
            }
            tr.open = false;
        }
        if (tr.room_num <= 7)
            tr.text_y += (208.0f / 2 + 4 - tr.text_y) * 0.05f;
    } else if (tr.close) {
        /* GML ran this lerp four times per frame (208/64 passes). */
        for (int i = 0; i < 4; i++) {
            if (tr.x >= -63.0f) {
                tr.x += (-64.0f - tr.x) * 0.02f;
            } else {
                tr.close = false;
            }
        }
        if (tr.room_num <= 7)
            tr.text_y += (208.0f + 16 - tr.text_y) * 0.16f;
    } else {
        tr.x = 304.0f;
        tr.text_y = -16.0f;
    }
}
