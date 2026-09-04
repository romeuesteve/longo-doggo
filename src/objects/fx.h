/*
 * FX object script: cosmetic particles consumed from the event queue —
 * smoke puffs, the "1" popups, bark wedges and sinking boxes.
 */
#ifndef LONGO_OBJECT_FX_H
#define LONGO_OBJECT_FX_H

#include "../core/view.h"
#include "../core/world.h"

void fx_reset(void);
void fx_tick(void);      /* consumes events, advances particles */
void fx_draw(void);      /* world layer */

#endif /* LONGO_OBJECT_FX_H */
