/*
 * Dialogue object script: the tutorial/credits speech boxes (oTutorial).
 * Gates the dog until every box has been advanced past.
 */
#ifndef LONGO_OBJECT_DIALOGUE_H
#define LONGO_OBJECT_DIALOGUE_H

#include <stdbool.h>

#define DIALOGUE_MAX_BOXES 8

/* Shrink ticks after the final press (lerp 0.15 below 0.65 scale). */
#define DIALOGUE_SHRINK_TICKS 34

typedef struct Dbox {
    float x, y;
    const char *text;
} Dbox;

void dialogue_reset(void);
/* Texts per room (tutorial, level6, level4, credits); clears the dog's
 * play gate through the dog script when the room gates play. */
void dialogue_start(int room_index);
void dialogue_tick(void); /* reads input through the world */

bool dialogue_active(void);
int dialogue_index(void);
const Dbox *dialogue_box(int index);
bool dialogue_released(void);

/* View. */
void dialogue_view_tick(void);
void dialogue_draw(void); /* GUI layer */

#endif /* LONGO_OBJECT_DIALOGUE_H */
