/*
 * Butterfly object script: ambient wanderer (oButterfly port).  Purely
 * cosmetic — placed from the room's decor instances.
 */
#ifndef LONGO_OBJECT_BUTTERFLY_H
#define LONGO_OBJECT_BUTTERFLY_H

#include "../core/world.h"

void butterfly_reset(void);
void butterfly_place(float x, float y);
void butterfly_tick(const SimInput *input);
void butterfly_draw(int shadow);

#endif /* LONGO_OBJECT_BUTTERFLY_H */
