/*
 * Flower object script: decorative swaying flowers placed from the room's
 * decor instances.  The dog's animation clock follows theirs.
 */
#ifndef LONGO_OBJECT_FLOWER_H
#define LONGO_OBJECT_FLOWER_H

void flower_reset(void);
void flower_place(float x, float y);
void flower_draw(int shadow);

#endif /* LONGO_OBJECT_FLOWER_H */
