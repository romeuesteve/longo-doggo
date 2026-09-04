#include "dialogue.h"

#include <string.h>

#include "../core/world.h"
#include "items.h"
#include "transition.h"

typedef struct Dialogue {
    bool active;
    int index;         /* current box (i) */
    int last;          /* last box index (num) */
    int release_ticks; /* ticks since the final press (-1 = not released) */
    bool gates_play;   /* this room's dialogue pauses the dog */
    float base_scale;  /* box pop-in target scale (4, or 10 on credits) */
    Dbox box[DIALOGUE_MAX_BOXES];
} Dialogue;

static Dialogue dlg;

/* Texts ported verbatim from the oTutorial create event. */
static const char *const TUTORIAL_TEXTS[7] = {
    "This is Longo Doggo",
    "He wants to enter his house, but he's too long so there's no room for him",
    "Apples make Longo Doggo longer",
    "Instead, pears make him shorter",
    "The house number shows how many length units you must lose",
    "If you get stuck press 'R' to retry",
    "Control Longo Doggo with the arrow keys"
};
static const float TUTORIAL_BOX_X[7] = { 103, 136, 198, 0, 144, 103, 103 };
static const float TUTORIAL_BOX_Y[7] = { 80, 56, 119, 119, 56, 80, 80 };

static const char *const LEVEL6_TEXTS[3] = {
    "This is a door",
    "It only opens once all the buttons are pressed simultaneously",
    "Buttons can be pressed either by Longo Doggo or boxes, which you can push on the sides"
};
static const float LEVEL6_BOX_X[3] = { 247, 174, 102 };
static const float LEVEL6_BOX_Y[3] = { 51, 120, 65 };

static const char *const LEVEL4_TEXTS[1] = {
    "Holes will prevent you from advancing unless you fill them with something"
};
static const float LEVEL4_BOX_X[1] = { 119 };
static const float LEVEL4_BOX_Y[1] = { 37 };

static const char *const CREDITS_TEXTS[2] = {
    "Thank you for playing! \n \n Game made by Romeu Esteve (@Romeuski) for the 'Tu juego a juicio Jam 2021' \n Using Game Maker Studio 2, freesound.org and Ableton Live 10",
    "If you enjoyed the experience please leave a comment in the itch.io page, I love feedback!"
};
static const float CREDITS_BOX_X[2] = { 151, 151 };
static const float CREDITS_BOX_Y[2] = { 37, 37 };

void dialogue_reset(void)
{
    memset(&dlg, 0, sizeof(dlg));
    dlg.release_ticks = -1;
}

void dialogue_start(int room_index)
{
    memset(&dlg, 0, sizeof(dlg));
    dlg.active = true;
    dlg.index = 0;
    dlg.release_ticks = -1;
    dlg.base_scale = 4;

    if (room_index == SIM_ROOM_TUTORIAL) {
        dlg.last = 6;
        dlg.gates_play = true;
        for (int i = 0; i <= dlg.last; i++) {
            dlg.box[i].x = TUTORIAL_BOX_X[i];
            dlg.box[i].y = TUTORIAL_BOX_Y[i];
            dlg.box[i].text = TUTORIAL_TEXTS[i];
        }
        /* box 3 sits beside the first skull (skull->x - 8) */
        for (int i = 0; i < skull_count(); i++) {
            if (skull_alive(i)) {
                dlg.box[3].x =
                    (float)(sim_cell_x(skull_cell(i)) * SIM_CELL - 8);
                break;
            }
        }
    } else if (room_index == SIM_ROOM_LEVEL6) {
        dlg.last = 2;
        dlg.gates_play = true;
        for (int i = 0; i <= dlg.last; i++) {
            dlg.box[i].x = LEVEL6_BOX_X[i];
            dlg.box[i].y = LEVEL6_BOX_Y[i];
            dlg.box[i].text = LEVEL6_TEXTS[i];
        }
    } else if (room_index == SIM_ROOM_LEVEL4) {
        dlg.last = 0;
        dlg.gates_play = true;
        dlg.box[0].x = LEVEL4_BOX_X[0];
        dlg.box[0].y = LEVEL4_BOX_Y[0];
        dlg.box[0].text = LEVEL4_TEXTS[0];
    } else if (room_index == SIM_ROOM_CREDITS) {
        dlg.last = 1;
        dlg.gates_play = false;
        dlg.base_scale = 10;
        for (int i = 0; i <= dlg.last; i++) {
            dlg.box[i].x = CREDITS_BOX_X[i];
            dlg.box[i].y = CREDITS_BOX_Y[i];
            dlg.box[i].text = CREDITS_TEXTS[i];
        }
    } else {
        dlg.active = false;
    }
    if (dlg.gates_play) world_ptr()->dog.play = 0;
}

/* oTutorial Draw event port; the box scale easing is view-side. */
void dialogue_tick(void)
{
    if (!dlg.active) return;
    const SimInput *input = &world_ptr()->input;
    if (dlg.release_ticks >= 0) {
        if (++dlg.release_ticks >= DIALOGUE_SHRINK_TICKS) dlg.active = false;
        return;
    }
    if ((input->pressed_space || input->pressed_enter || input->pressed_e) &&
        !transition_closing()) {
        if (dlg.index < dlg.last) {
            dlg.index++;
        } else {
            dlg.release_ticks = 0;
            if (dlg.gates_play) world_ptr()->dog.play = 1;
        }
    }
}

bool dialogue_active(void) { return dlg.active; }
int dialogue_index(void) { return dlg.index; }

const Dbox *dialogue_box(int index)
{
    if (index < 0 || index >= DIALOGUE_MAX_BOXES) return NULL;
    return &dlg.box[index];
}

float dialogue_base_scale(void) { return dlg.base_scale; }
bool dialogue_released(void) { return dlg.release_ticks >= 0; }
