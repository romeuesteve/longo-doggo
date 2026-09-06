#include "transition.h"

#include <stdio.h>

#include "../core/world.h"
#include "../core/view.h"

static TransitionState tr;

void transition_reset(void)
{
    /* initial wipe state */
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

/* Draw pass, replayed as pushed. */
void transition_tick(SimWorld *w)
{
    if (!tr.active) return;
    if (tr.open) {
        if (tr.x >= -20.0f) {
            tr.x += (-31.0f - tr.x) * 0.04f;
        } else {
            tr.x = 304.0f;
            tr.close = true;
            if (tr.retry) {
                sim_room_restart(w);
                tr.retry = false;
            } else if (tr.next_lvl) {
                sim_room_goto_next(w);
                tr.next_lvl = false;
            }
            tr.open = false;
        }
        if (tr.room_num <= 7)
            tr.text_y += (208.0f / 2 + 4 - tr.text_y) * 0.05f;
    } else if (tr.close) {
        /* four convergence passes per tick close the wipe fast enough. */
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


/* ------------------------------------------------------------------ */
/* View: the level wipe + "LEVEL N" label (Draw GUI pass)              */
/* ------------------------------------------------------------------ */

void transition_draw(void)
{
    if (!tr.active) return;
    view_layer(VIEW_GUI);
    ViewColor color2 = view_rgb(113 - 10, 153 - 10, 61 - 10);
    ViewColor color = view_rgb(141 - 10, 199 - 10, 63 - 10);

    if (tr.open) {
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
    } else if (tr.close) {
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

    if (tr.room_num <= 7 && (tr.open || tr.close)) {
        char text[128];
        snprintf(text, sizeof(text), "LEVEL %d", tr.room_num);
        view_text(0, 0, text, 142.0f, tr.text_y + 2.0f, 1.0f,
                  view_rgb(0, 128, 0));
        view_text(0, 0, text, 140.0f, tr.text_y, 1.0f,
                  view_rgb(255, 255, 255));
    }
}
