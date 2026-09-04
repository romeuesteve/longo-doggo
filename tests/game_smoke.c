/*
 * Headless smoke tests for the GameMaker-faithful simulation core.
 * Runs the recovered rooms with deterministic input and asserts the
 * recovered GML mechanics (movement cadence, length, push, holes,
 * buttons/doors, transitions, runtime room order).
 */
#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static LongoWorld world;

static void tick_with(const LongoInput *input)
{
    longo_tick(&world, input, 1000.0 / 60.0);
}

static void tick_idle(int n)
{
    LongoInput none;
    memset(&none, 0, sizeof(none));
    for (int i = 0; i < n; i++) longo_tick(&world, &none, 1000.0 / 60.0);
}

static LongoInst *dog(void)
{
    return longo_find_first(&world, LONGO_OBJ_DOG);
}

static void press(unsigned char *flag)
{
    *flag = 1;
}

/* ---------------------------------------------------------------- */

static void test_title_flow_and_room_order(void)
{
    LongoInput input;
    memset(&input, 0, sizeof(input));

    longo_init(&world, 42u);
    assert(world.room_index == 0);
    assert(strcmp(world.room->name, "rm_title_screen") == 0);
    assert(longo_instance_exists(&world, LONGO_OBJ_TITLE));
    /* oTransition is persistent and placed in the title room */
    assert(longo_instance_exists(&world, LONGO_OBJ_TRANSITION));
    assert(dog() != NULL);
    assert(dog()->length == 5);

    press(&input.vk_anykey);
    tick_with(&input);
    tick_idle(200);
    assert(world.room_index == 1);
    assert(strcmp(world.room->name, "rm_tutorial") == 0);
    /* "LEVEL 1" transition ran with room_num 1 */
    assert(longo_find_first(&world, LONGO_OBJ_TRANSITION)->room_num == 1);
}

static void test_tutorial_dialogue_gates_play(void)
{
    LongoInput input;
    memset(&input, 0, sizeof(input));

    assert(dog()->play == 0); /* oTutorial paused the dog */
    for (int i = 0; i < 6; i++) {
        press(&input.key_space);
        tick_with(&input);
        input.key_space = 0;
        tick_idle(1);
    }
    /* i == num: next press releases the dog */
    assert(dog()->play == 0);
    press(&input.key_space);
    tick_with(&input);
    tick_idle(40); /* box shrink animation, then destroyed */
    assert(dog()->play == 1);
    assert(!longo_instance_exists(&world, LONGO_OBJ_TUTORIAL));
}

static void test_movement_and_cooldown(void)
{
    LongoInput input;
    memset(&input, 0, sizeof(input));
    float start_x = dog()->x;

    /* oDog Step runs only once per tick like GameMaker */
    press(&input.vk_right);
    tick_with(&input);
    assert(dog()->x == start_x + 16.0f);
    assert(dog()->sprite_index == LONGO_SPR_DOGRIGHT);
    assert(dog()->key_cooldown == 0);

    /* held key repeats but the 2-tick alarm gates the second move */
    press(&input.vk_right);
    tick_with(&input);
    assert(dog()->x == start_x + 16.0f); /* key_cooldown still 0 */
    tick_idle(1);                        /* alarm[1] fires: cooldown 1 */
    press(&input.vk_right);
    tick_with(&input);
    assert(dog()->x == start_x + 32.0f);

    /* the body follows into the head's previous cells */
    assert(dog()->ins[0] >= 0);
    LongoInst *part0 = longo_find_id(&world, dog()->ins[0]);
    assert(part0 != NULL);
    tick_idle(4); /* parts snap via follow->xprev */
    part0 = longo_find_id(&world, dog()->ins[0]);
    assert(part0->x == start_x + 16.0f);
    assert(dog()->image_speed == 1.0f);
}

static void test_apple_and_skull_length(void)
{
    LongoInst *d = dog();
    LongoInst *apple = NULL;
    LongoInst *skull = NULL;

    /* teleport beside the tutorial apple at (192,144) (bbox cell centre) */
    apple = NULL;
    for (int i = 0; i < world.instance_count; i++) {
        if (world.instances[i].alive &&
            world.instances[i].object == LONGO_OBJ_APPLE &&
            world.instances[i].x == 192.0f && world.instances[i].y == 144.0f)
            apple = &world.instances[i];
    }
    assert(apple != NULL);
    d->x = apple->x - 16.0f;
    d->y = apple->y + 8.0f; /* cell centre of the row below? no: same row */
    /* place exactly one cell to the left, same cell row (centres) */
    d->x = apple->x - 16.0f;
    d->y = apple->y + 8.0f;
    apple->y = d->y - 8.0f; /* align apple cell to the dog row */
    int length_before = d->length;

    LongoInput input;
    memset(&input, 0, sizeof(input));
    press(&input.vk_right);
    tick_with(&input);
    tick_idle(1);
    assert(dog()->length == length_before + 1);
    assert(apple->alive == 0);

    /* skull at length 3 shortens; at length 2 it kills the dog */
    for (int i = 0; i < world.instance_count; i++) {
        if (world.instances[i].alive &&
            world.instances[i].object == LONGO_OBJ_SKULL)
            skull = &world.instances[i];
    }
    assert(skull != NULL);
    d = dog();
    d->length = 3;
    d->x = skull->x - 16.0f;
    d->y = skull->y + 8.0f;
    skull->y = d->y - 8.0f;
    press(&input.vk_right);
    tick_with(&input);
    tick_idle(1);
    assert(dog() != NULL);
    assert(dog()->length == 2);

    for (int i = 0; i < world.instance_count; i++) {
        if (world.instances[i].alive &&
            world.instances[i].object == LONGO_OBJ_SKULL)
            skull = &world.instances[i];
    }
    assert(skull != NULL);
    d = dog();
    d->x = skull->x - 16.0f;
    d->y = skull->y + 8.0f;
    skull->y = d->y - 8.0f;
    press(&input.vk_right);
    tick_with(&input);
    tick_idle(1);
    /* eating a pear at length 2 destroys the dog (and the parts follow) */
    assert(dog() == NULL);
}

static void test_box_push_hole_button_door(void)
{
    /* rm_level1 (runtime index 6) exercises boxes, holes, buttons, doors */
    longo_room_goto(&world, 6);
    assert(strcmp(world.room->name, "rm_level1") == 0);
    tick_idle(2);

#ifdef DEBUG_SPAWN
    {
        int counts[32] = { 0 };
        for (size_t i = 0; i < (size_t)world.instance_count; i++) {
            if (world.instances[i].alive)
                counts[world.instances[i].object]++;
        }
        for (int o = 0; o < 28; o++) {
            if (counts[o]) printf("obj %d alive: %d\n", o, counts[o]);
        }
        fflush(stdout);
    }
#endif
    LongoInst *box = NULL;
    LongoInst *hole = NULL;
    for (int i = 0; i < world.instance_count; i++) {
        LongoInst *inst = &world.instances[i];
        if (!inst->alive) continue;
        if (inst->object == LONGO_OBJ_BOX && box == NULL &&
            inst->x == 96.0f && inst->y == 80.0f)
            box = inst;
        if (inst->object == LONGO_OBJ_HOLE && hole == NULL &&
            inst->x == 208.0f && inst->y == 80.0f)
            hole = inst;
    }
    assert(box != NULL);
    assert(hole != NULL);
    /* GML: boxes carry push=1; block is recomputed every Step and is 0
     * while nothing jams the box against a blocker. */
    assert(box->push == 1);
    assert(box->block == 0);
    assert(hole->block == 1 && hole->full == 0);

    /* a box lerps into its logical cell when pushed */
    box->xx = hole->x;
    box->yy = hole->y;
    tick_idle(30);
    assert(hole->full == 1);
    assert(hole->block == 0);
    assert(box->alive == 0); /* destroyed by the oHole collision */

    /* Buttons count: rm_level1 has four.  Press three with boxes (a box
     * lerps onto the button cell) and the last one with the dog head. */
    struct { float bx, by, ox, oy; } pushes[3] = {
        { 224.0f, 144.0f, 224.0f, 128.0f },
        { 240.0f, 144.0f, 240.0f, 128.0f },
        { 192.0f, 128.0f, 208.0f, 160.0f },
    };
    for (int i = 0; i < 3; i++) {
        LongoInst *b = NULL;
        for (int j = 0; j < world.instance_count; j++) {
            LongoInst *inst = &world.instances[j];
            if (inst->alive && inst->object == LONGO_OBJ_BOX &&
                inst->xx == pushes[i].ox && inst->yy == pushes[i].oy)
                b = inst;
        }
        assert(b != NULL);
        b->xx = pushes[i].bx;
        b->yy = pushes[i].by;
        tick_idle(30);
    }
    LongoInst *button4 = NULL;
    for (int i = 0; i < world.instance_count; i++) {
        LongoInst *inst = &world.instances[i];
        if (inst->alive && inst->object == LONGO_OBJ_BUTTON &&
            inst->x == 256.0f && inst->y == 64.0f)
            button4 = inst;
    }
    assert(button4 != NULL);
    {
        LongoInst *d = dog();
        assert(d != NULL);
        d->x = button4->x + 8.0f;
        d->y = button4->y + 8.0f;
        d->dir = 180;
    }
    tick_idle(3);
    assert(button4->pressed == 1);
    assert(world.g_buttons == longo_instance_number(&world, LONGO_OBJ_BUTTON));

    /* doors open once every button is pressed simultaneously */
    tick_idle(1);
    assert(longo_find_first(&world, LONGO_OBJ_DOOR)->open == 1);
}

static void test_win_advances_to_next_runtime_room(void)
{
    /* rm_level1 (index 6) is followed by rm_level3 (index 7) */
    LongoInst *d = dog();
    LongoInst *win = NULL;
    LongoInst *goalup = longo_find_first(&world, LONGO_OBJ_GOALUP);
    assert(goalup != NULL);
    assert(goalup->remain == 3);

    for (int i = 0; i < world.instance_count; i++) {
        if (world.instances[i].alive &&
            world.instances[i].object == LONGO_OBJ_WIN)
            win = &world.instances[i];
    }
    assert(win != NULL);
    d->length = 2;
    goalup->remain = 0; /* the oGoalUp draw event keeps this updated */
    d->x = win->x + 8.0f;
    d->y = win->y + 8.0f;
    tick_idle(2);
    assert(longo_find_first(&world, LONGO_OBJ_TRANSITION)->next_lvl == 1);

    tick_idle(200);
    assert(world.room_index == 7);
    assert(strcmp(world.room->name, "rm_level3") == 0);
    /* room_num counts wins, not room indices: it was 1 before this win */
    assert(longo_find_first(&world, LONGO_OBJ_TRANSITION)->room_num == 2);
}

static void test_retry_reloads_room(void)
{
    int room_before = world.room_index;
    LongoInput input;
    memset(&input, 0, sizeof(input));
    press(&input.key_r);
    tick_with(&input);
    tick_idle(200);
    assert(world.room_index == room_before);
    assert(dog() != NULL);
    assert(dog()->length == 5);
}

int main(void)
{
    test_title_flow_and_room_order();
    test_tutorial_dialogue_gates_play();
    test_movement_and_cooldown();
    test_apple_and_skull_length();
    test_box_push_hole_button_door();
    test_win_advances_to_next_runtime_room();
    test_retry_reloads_room();
    printf("longo_game_smoke: all tests passed\n");
    return 0;
}
