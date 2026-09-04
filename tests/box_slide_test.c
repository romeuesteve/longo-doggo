/*
 * Headless trace of box sliding in rm_level6 (runtime room 2), replicating
 * the recovered GML probe semantics (+10 probe for block, +8 probe for push).
 * Dog starts one cell left of the box at (208,16) and pushes right.
 */
#include "game.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    static LongoWorld w; /* several MB: keep off the stack */
    LongoInput in;
    LongoInst *dog, *box;
    int i;

    memset(&in, 0, sizeof(in));
    longo_init(&w, 7u);
    longo_room_goto(&w, 2); /* rm_level6 */
    for (i = 0; i < 3; i++) longo_tick(&w, &in, 1000.0 / 60.0);

    dog = longo_find_first(&w, LONGO_OBJ_DOG);
    box = NULL;
    for (i = 0; i < w.instance_count; i++) {
        if (w.instances[i].alive && w.instances[i].object == LONGO_OBJ_BOX &&
            w.instances[i].xx == 96.0f && w.instances[i].yy == 96.0f)
            box = &w.instances[i];
    }
    if (!dog || !box) {
        printf("FAIL: dog or box missing\n");
        return 1;
    }
    dog->play = 1; /* skip the oTutorial dialogue gate */
    /* stand above the box at (96,96) and push it down the clear column */
    dog->x = 104.0f;
    dog->y = 88.0f;
    dog->xx = 104.0f;
    dog->yy = 88.0f;
    dog->xprev = 104.0f;
    dog->yprev = 88.0f;
    dog->dir = 0;
    dog->sprite_index = LONGO_SPR_DOGDOWN;

    printf("tick  dog.y  box.y  box.yy  block  cooldown\n");
    for (i = 0; i < 24; i++) {
        in.vk_down = 1;
        longo_tick(&w, &in, 1000.0 / 60.0);
        in.vk_down = 0;
        longo_tick(&w, &in, 1000.0 / 60.0); /* let cooldown tick down */
        printf("%4d  %5.1f  %5.2f  %5.1f  %4d  %d\n", i, dog->y, box->y,
               box->yy, box->block, dog->key_cooldown);
    }
    return 0;
}
