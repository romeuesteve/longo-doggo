#include "title.h"

#include "../core/events.h"
#include "../core/view.h"
#include "../core/world.h"
#include "transition.h"

static bool present;

void title_reset(void) { present = false; }

void title_place(void)
{
    present = true;
    events_sound(SND_PLACEHOLDER, 1); /* the looping music track */
}

void title_tick(const SimInput *input)
{
    if (!present) return;
    if (input->pressed_any) transition_request_next();
}

bool title_present(void) { return present; }


/* ------------------------------------------------------------------ */
/* View: waving title logo (world) + its shadow                        */
/* ------------------------------------------------------------------ */

void title_draw(int shadow)
{
    if (!present) return;
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor tint = shadow ? view_rgb(0, 0, 0) : view_rgb(255, 255, 255);
    float alpha = shadow ? 0.5f : 1.0f;
    /* title placement: x = 304/4 - 20, y = 208/4 - 20 */
    float x = 304.0f / 4.0f - 20.0f;
    float y = 208.0f / 4.0f - 20.0f;
    float wave1 = longo_wave(0, 8, 2, 0, view_time_ms());
    float wave2 = longo_wave(0, 8, 2, 0.1f, view_time_ms());
    int depth = shadow ? 0 : VIEW_DEPTH_TITLE;

    if (shadow) {
        view_sprite_part(depth, LONGO_SPR_TITLE, 0, 0, 0, 191, 64, x,
                         y + wave1 + 85.0f, 1.0f, 0.5f, tint, alpha);
        view_sprite_part(depth, LONGO_SPR_TITLE, 0, 0, 69, 191, 149, x,
                         y + 48.0f + wave2 + 52.0f, 1.0f, 0.5f, tint, alpha);
        return;
    }
    view_sprite_part(depth, LONGO_SPR_TITLE, 0, 0, 0, 191, 64, x,
                     y + wave1, 1.0f, 1.0f, tint, alpha);
    view_sprite_part(depth, LONGO_SPR_TITLE, 0, 0, 69, 191, 149, x,
                     y + 48.0f + wave2 + 2.0f, 1.0f, 1.0f, tint, alpha);
    /* "Press Any Key to Start", gradient shadowed */
    view_text(depth, 0, "Press Any Key to Start", 151.0f,
              188.0f + wave1 * 0.5f + 1.0f, 1.0f, view_rgb(51, 17, 0));
    view_text(depth, 0, "Press Any Key to Start", 150.0f,
              188.0f + wave1 * 0.5f, 1.0f, view_rgb(255, 235, 204));
}
