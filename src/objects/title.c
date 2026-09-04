#include "title.h"

#include "../core/events.h"
#include "../core/world.h"
#include "transition.h"

static bool present;

void title_reset(void) { present = false; }

void title_place(void)
{
    present = true;
    events_sound(SND_PLACEHOLDER, 1); /* the looping music track */
}

void title_tick(void)
{
    if (!present) return;
    if (world_ptr()->input.pressed_any) transition_request_next();
}

bool title_present(void) { return present; }
