#include "transition.h"

#include <stdio.h>

#include "../core/world.h"
#include "../core/view.h"

static TransitionState tr;

void transition_reset(void)
{
    /* initial wipe state */
    tr.phase = TRANSITION_IDLE;
    tr.pending = TRANSITION_ACTION_NONE;
    tr.x = 304.0f;
    tr.text_y = -16.0f;
    tr.room_num = 1;
}

const TransitionState *transition_state(void) { return &tr; }

bool transition_closing(void) { return tr.phase == TRANSITION_CLOSING; }

void transition_request_retry(void)
{
    /* one action per wipe: the retry replaces any pending advance and
     * reverses the increment its win made, so the room reloads with the
     * label it had before the win */
    if (tr.pending == TRANSITION_ACTION_NEXT_ROOM) tr.room_num--;
    tr.pending = TRANSITION_ACTION_RETRY;
    tr.phase = TRANSITION_OPENING;
}

void transition_request_next(void)
{
    /* one action per wipe: the advance replaces a pending retry too,
     * and it keeps the increment its win just made — the exact mirror
     * of request_retry's reversal, so room_num always reads one past
     * the last room whose advance is pending or was applied */
    tr.pending = TRANSITION_ACTION_NEXT_ROOM;
    tr.phase = TRANSITION_OPENING;
}

void transition_count_win(void) { tr.room_num++; }

/* Draw pass, replayed as pushed. */
void transition_tick(SimWorld *w)
{
    switch (tr.phase) {
    case TRANSITION_OPENING:
        if (tr.x >= -20.0f) {
            tr.x += (-31.0f - tr.x) * 0.04f;
        } else {
            /* midpoint: the screen is covered; apply the pending room
             * change and start uncovering the (new) room */
            tr.x = 304.0f;
            tr.phase = TRANSITION_CLOSING;
            if (tr.pending == TRANSITION_ACTION_RETRY)
                sim_room_restart(w);
            else if (tr.pending == TRANSITION_ACTION_NEXT_ROOM)
                sim_room_goto_next(w);
            tr.pending = TRANSITION_ACTION_NONE;
        }
        if (tr.room_num <= 7)
            tr.text_y += (208.0f / 2 + 4 - tr.text_y) * 0.05f;
        break;
    case TRANSITION_CLOSING:
        /* four convergence passes per tick close the wipe fast enough. */
        for (int i = 0; i < 4; i++) {
            if (tr.x >= -63.0f) {
                tr.x += (-64.0f - tr.x) * 0.02f;
            } else {
                tr.phase = TRANSITION_IDLE;
            }
        }
        if (tr.room_num <= 7)
            tr.text_y += (208.0f + 16 - tr.text_y) * 0.16f;
        break;
    case TRANSITION_IDLE:
    default:
        tr.x = 304.0f;
        tr.text_y = -16.0f;
        break;
    }
}


/* ------------------------------------------------------------------ */
/* View: the level wipe + "LEVEL N" label (Draw GUI pass)              */
/* ------------------------------------------------------------------ */

void transition_draw(void)
{
    view_layer(VIEW_GUI);
    ViewColor color2 = view_rgb(113 - 10, 153 - 10, 61 - 10);
    ViewColor color = view_rgb(141 - 10, 199 - 10, 63 - 10);

    if (tr.phase == TRANSITION_OPENING) {
        for (int i = 0; i < 4; i++) {
            view_sprite(2, LONGO_SPR_TRANSITION, 0, tr.x,
                        (float)(i * 64) + 7.0f, 1.0f, 1.0f, 0.0f, color2,
                        1.0f);
            view_sprite(2, LONGO_SPR_TRANSITION, 0, tr.x,
                        (float)(i * 64), 1.0f, 1.0f, 0.0f, color, 1.0f);
        }
        if (tr.x + 32.0f > 0.0f)
            view_rect(1, tr.x + 32.0f, 0.0f, 304.0f - (tr.x + 32.0f),
                      208.0f, color);
    } else if (tr.phase == TRANSITION_CLOSING) {
        for (int i = 0; i < 4; i++) {
            view_sprite(2, LONGO_SPR_TRANSITION, 1, tr.x,
                        (float)(i * 64) + 7.0f, 1.0f, 1.0f, 0.0f, color2,
                        1.0f);
            view_sprite(2, LONGO_SPR_TRANSITION, 1, tr.x,
                        (float)(i * 64), 1.0f, 1.0f, 0.0f, color, 1.0f);
        }
        if (tr.x + 32.0f > 0.0f)
            view_rect(1, 0.0f, 0.0f, tr.x + 32.0f, 208.0f, color);
    }

    if (tr.room_num <= 7 && tr.phase != TRANSITION_IDLE) {
        char text[128];
        snprintf(text, sizeof(text), "LEVEL %d", tr.room_num);
        view_text(0, 1, text, 152.0f, tr.text_y + 2.0f, 1.0f,
                  view_rgb(0, 128, 0));
        view_text(0, 1, text, 152.0f, tr.text_y, 1.0f,
                  view_rgb(255, 255, 255));
    }
}
