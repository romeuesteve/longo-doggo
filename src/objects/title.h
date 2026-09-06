/*
 * Title object script: the title screen flag.  Any key starts the game.
 */
#ifndef LONGO_OBJECT_TITLE_H
#define LONGO_OBJECT_TITLE_H

#include <stdbool.h>

#include "../core/world.h"

void title_reset(void);
void title_place(void); /* starts the music, marks the room as a title */
void title_tick(const SimInput *input); /* any key -> next-level wipe */

bool title_present(void);

/* View. */
void title_draw(int shadow);

#endif /* LONGO_OBJECT_TITLE_H */
