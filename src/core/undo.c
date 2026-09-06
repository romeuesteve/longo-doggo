#include "undo.h"

#include "../objects/box.h"
#include "../objects/door.h"
#include "../objects/dog.h"
#include "../objects/hole.h"
#include "../objects/house.h"
#include "../objects/items.h"
#include "events.h"
#include "solid.h"

/* Every board-changing move is one dog step; ~4 KB each, so a few
 * hundred entries stay well inside static memory. */
#define UNDO_MAX 256

typedef struct UndoState {
    Dog dog;
    BoxSnapshot box;
    HoleSnapshot hole;
    ItemsSnapshot items;
    DoorSnapshot door;
    House house;
    SolidSnapshot solid;
} UndoState;

/* Ring buffer: when full, the oldest entry is silently dropped (undo
 * has finite memory; a level never needs more than UNDO_MAX steps). */
static UndoState history[UNDO_MAX];
static int history_start; /* oldest entry */
static int history_count;
static UndoState scratch;
static bool scratch_live;

void undo_reset(void)
{
    history_start = 0;
    history_count = 0;
    scratch_live = false;
}

void undo_begin_step(void)
{
    dog_capture(&scratch.dog);
    box_capture(&scratch.box);
    hole_capture(&scratch.hole);
    items_capture(&scratch.items);
    door_capture(&scratch.door);
    house_capture(&scratch.house);
    solid_capture(&scratch.solid);
    scratch_live = true;
}

void undo_commit_step(void)
{
    if (!scratch_live) return;
    scratch_live = false;
    history[(history_start + history_count) % UNDO_MAX] = scratch;
    if (history_count == UNDO_MAX)
        history_start = (history_start + 1) % UNDO_MAX;
    else
        history_count++;
}

bool undo_pop(void)
{
    if (history_count == 0) return false;
    history_count--;
    const UndoState *s = &history[(history_start + history_count) % UNDO_MAX];
    dog_restore(&s->dog);
    box_restore(&s->box);
    hole_restore(&s->hole);
    items_restore(&s->items);
    door_restore(&s->door);
    house_restore(&s->house);
    solid_restore(&s->solid);
    events_sound(SND_POOF, 0);
    return true;
}

bool undo_tick(const SimInput *input)
{
    if (!input->pressed_undo) return false;
    return undo_pop();
}
