/*
 * Headless full-flow driver: replays the exact input sequence that the
 * live verification used (any key on the title, seven space presses at
 * 250 ms, then holding right) and reports the resulting states, so the
 * meta flow can be verified without rendering.
 */
#include "core/world.h"

#include <stdio.h>
#include <string.h>

#include "objects/dialogue.h"
#include "objects/dog.h"
#include "objects/house.h"
#include "objects/items.h"
#include "objects/title.h"
#include "objects/transition.h"

#define world (*world_ptr())

static void tick(const SimInput *in) { sim_tick(world_ptr(), in); }

int main(void)
{
    SimInput in;
    memset(&in, 0, sizeof(in));

    sim_init(42u);
    printf("t=0 room=%d (%s)\n", world.room_index, world.room->name);

    /* any key on the title, then spaces starting 0.45 s later (27 ticks,
     * i.e. during the open wipe) - the exact live-input timing */
    in.pressed_any = 1;
    tick(&in);
    in.pressed_any = 0;
    for (int i = 0; i < 27; i++) tick(&in);
    printf("after any key + 27 ticks: room=%d (%s) trans(phase=%d num=%d)\n",
           world.room_index, world.room->name,
           (int)transition_state()->phase, transition_state()->room_num);

    /* seven spaces at 250 ms (15 tick) intervals */
    for (int i = 0; i < 7; i++) {
        in.pressed_space = 1;
        tick(&in);
        in.pressed_space = 0;
        for (int t = 0; t < 14; t++) tick(&in);
        printf("space %d: dialogue(active=%d idx=%d rel=%d) dog_play=%d "
               "trans(close=%d) room=%d\n",
               i, dialogue_active() ? 1 : 0, dialogue_index(),
               dialogue_released() ? 1 : 0, dog_play() ? 1 : 0,
               transition_closing() ? 1 : 0, world.room_index);
    }

    /* tap right every 10 ticks for 3 seconds (movement is one cell per
     * press, not while held), reporting every half second */
    for (int t = 0; t < 180; t++) {
        in.pressed_right = (t % 10 == 0);
        tick(&in);
        if (t % 30 == 0) {
            printf("hold r t=%d: dog=(%d,%d) len=%d alive=%d room=%d "
                   "trans(phase=%d num=%d) win_ready=%d remain=%d\n",
                   t, dog_cx(), dog_cy(), dog_length(), dog_alive() ? 1 : 0,
                   world.room_index, (int)transition_state()->phase,
                   transition_state()->room_num,
                   house_win_ready() ? 1 : 0, house_remain());
        }
    }
    printf("final: room=%d (%s) num=%d dog=(%d,%d) len=%d alive=%d\n",
           world.room_index, world.room->name, transition_state()->room_num,
           dog_cx(), dog_cy(), dog_length(), dog_alive() ? 1 : 0);
    return 0;
}
