/*
 * Headless smoke tests for the simulation core.
 * Runs the recovered rooms with deterministic input and asserts the ported
 * rules: runtime room order, dialogue gating, tile-based movement cadence,
 * chain follow, apple/skull length changes, box push/hole fill,
 * simultaneous button/door logic, the win transition order, and retry.
 */
#include "core/world.h"

#include "objects/box.h"
#include "objects/house.h"
#include "objects/dialogue.h"
#include "objects/items.h"
#include "objects/transition.h"
#include "objects/title.h"
#include "objects/hole.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define world (*world_ptr())

static void tick_with(const SimInput *input) { sim_tick(&world, input); }

static void tick_idle(int n)
{
    SimInput none;
    memset(&none, 0, sizeof(none));
    for (int i = 0; i < n; i++) sim_tick(&world, &none);
}

static SimDog *dog(void) { return &world.dog; }

static int box_at_cell(int cx, int cy)
{
    return box_index_at(sim_cell_of(cx, cy));
}

static int hole_at_cell(int cx, int cy)
{
    return hole_index_at(sim_cell_of(cx, cy));
}

/* ---------------------------------------------------------------- */

static void test_title_flow_and_room_order(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));

    sim_init(42u);
    assert(world.room_index == SIM_ROOM_TITLE);
    assert(strcmp(world.room->name, "rm_title_screen") == 0);
    assert(title_present());
    assert(transition_state()->active); /* persistent from the title room */
    assert(dog()->alive);
    assert(dog()->length == 5);

    input.pressed_any = 1;
    tick_with(&input);
    tick_idle(200);
    assert(world.room_index == SIM_ROOM_TUTORIAL);
    assert(strcmp(world.room->name, "rm_tutorial") == 0);
    /* "LEVEL 1" transition ran with room_num 1 */
    assert(transition_state()->room_num == 1);
}

static void test_tutorial_dialogue_gates_play(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));

    assert(dog()->play == 0); /* the tutorial paused the dog */
    for (int i = 0; i < 6; i++) {
        input.pressed_space = 1;
        tick_with(&input);
        input.pressed_space = 0;
        tick_idle(1);
    }
    /* index == last: the next press releases the dog */
    assert(dog()->play == 0);
    input.pressed_space = 1;
    tick_with(&input);
    tick_idle(SIM_DIALOGUE_SHRINK_TICKS + 10); /* box shrink, then gone */
    assert(dog()->play == 1);
    assert(!dialogue_active());
}

static void test_movement_and_chain(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));
    int start_x = dog()->cx;
    int start_y = dog()->cy;
    uint16_t head0 = sim_cell_of(start_x, start_y);

    /* a fresh press steps instantly and snaps the head cell */
    input.held_right = 1;
    tick_with(&input);
    assert(dog()->cx == start_x + 1);
    assert(dog()->dir == 90);

    /* the held-repeat timer gates the next step (one step per 2 ticks) */
    tick_with(&input);
    assert(dog()->cx == start_x + 1); /* move_timer still counting */
    tick_idle(1);                     /* timer expires */
    tick_with(&input);
    assert(dog()->cx == start_x + 2);

    /* the body follows into the head's previous cells */
    tick_idle(3);
    assert(dog()->chain[0] == sim_cell_of(start_x + 1, start_y));
    assert(dog()->chain[1] == head0);
    assert(world.dog.play);
}

static void test_apple_and_skull_length(void)
{
    SimDog *d = dog();
    SimInput input;
    memset(&input, 0, sizeof(input));

    /* move the head beside the tutorial apple and step into it */
    int apple = -1;
    for (int i = 0; i < apple_count(); i++) {
        if (apple_alive(i)) {
            apple = i;
            break;
        }
    }
    assert(apple >= 0);
    d->cx = sim_cell_x(apple_cell(apple)) - 1;
    d->cy = sim_cell_y(apple_cell(apple));
    int length_before = d->length;

    input.held_right = 1;
    tick_with(&input);
    tick_idle(1);
    assert(dog()->length == length_before + 1);
    assert(apple_alive(apple) == false);
    /* the new tail segment sits on the cell the tail just left */
    assert(dog()->chain[dog()->length - 1] == dog()->detached_cell);

    /* a skull at length 3 shortens; at length 2 it kills the dog */
    int skull = -1;
    for (int i = 0; i < skull_count(); i++) {
        if (skull_alive(i)) {
            skull = i;
            break;
        }
    }
    assert(skull >= 0);
    d = dog();
    d->length = 3;
    d->cx = sim_cell_x(skull_cell(skull)) - 1;
    d->cy = sim_cell_y(skull_cell(skull));
    input.held_right = 1;
    tick_with(&input);
    tick_idle(1);
    assert(dog()->alive);
    assert(dog()->length == 2);

    for (int i = 0; i < skull_count(); i++) {
        if (skull_alive(i)) {
            skull = i;
            break;
        }
    }
    assert(skull >= 0);
    d = dog();
    d->cx = sim_cell_x(skull_cell(skull)) - 1;
    d->cy = sim_cell_y(skull_cell(skull));
    input.held_right = 1;
    tick_with(&input);
    tick_idle(1);
    /* eating a pear at length 2 destroys the dog (original behaviour) */
    assert(dog()->alive == 0);
}

static void test_walls_and_push_rules(void)
{
    /* rm_level1 (runtime index SIM_ROOM_LEVEL1) exercises boxes, holes,
     * buttons, doors */
    sim_room_goto(&world, SIM_ROOM_LEVEL1);
    assert(strcmp(world.room->name, "rm_level1") == 0);
    tick_idle(2);

    int box = box_at_cell(6, 5);
    int hole = hole_at_cell(13, 5);
    assert(box >= 0);
    assert(hole >= 0);
    assert(hole_is_full(hole) == false);

    /* a box pushed into the hole fills it and the box is destroyed */
    box_set_cell(box, sim_cell_of(12, 5)); /* one cell left of the hole */
    {
        SimInput input;
        memset(&input, 0, sizeof(input));
        SimDog *d = dog();
        d->cx = 11;
        d->cy = 5;
        d->dir = 90;
        input.held_right = 1;
        tick_with(&input);
    }
    assert(hole_is_full(hole));
    assert(box_alive(box) == false);
    assert(dog()->cx == 12); /* the dog took the box's old cell */

    /* walls block: stepping into a static block strains in place */
    SimInput input;
    memset(&input, 0, sizeof(input));
    {
        int cx = -1, cy = -1;
        for (int y = 0; y < world.cells_h && cx < 0; y++) {
            for (int x = 1; x < world.cells_w && cx < 0; x++) {
                if (world.solid[sim_cell_of(x, y)] &&
                    !world.solid[sim_cell_of(x - 1, y)]) {
                    cx = x - 1;
                    cy = y;
                }
            }
        }
        assert(cx >= 0);
        SimDog *d = dog();
        d->alive = 1;
        d->cx = cx;
        d->cy = cy;
        d->length = 5;
        d->move_timer = 0;
        for (int i = 0; i < d->length; i++)
            d->chain[i] = sim_cell_of(cx, cy + 1 + i);
        input.held_right = 1;
        int sx = cx, sy = cy;
        tick_with(&input);
        tick_idle(4);
        assert(world.dog.strain);
        assert(world.dog.cx == sx && world.dog.cy == sy);
    }
}

static void test_buttons_door_win_retry(void)
{
    /* rm_level1 has four buttons; doors open once all are pressed at once */
    sim_room_goto(&world, SIM_ROOM_LEVEL1);
    tick_idle(2);
    assert(world.doors[0].open == 0);

    /* park a distinct box on every button zone but the last one */
    for (int i = 0; i < world.button_count - 1; i++) {
        SimButton *b = &world.buttons[i];
        uint16_t cell = b->zone[b->zone_count / 2];
        int existing = box_index_at(cell);
        if (existing >= 0 && existing != i) box_set_cell(existing, sim_cell_of(0, 0));
        box_set_cell(i, cell);
        tick_idle(1);
    }
    tick_idle(1);
    {
        int pressed_count = 0;
        for (int i = 0; i < world.button_count - 1; i++)
            if (world.buttons[i].pressed) pressed_count++;
        assert(pressed_count == world.button_count - 1);
    }
    /* doors stay closed until every button is pressed simultaneously */
    assert(world.buttons_pressed == world.button_count - 1);
    assert(world.doors[0].open == 0);

    /* the last button: a box pressed through its zone opens the doors */
    {
        SimButton *b = &world.buttons[world.button_count - 1];
        uint16_t cell = b->box_zone[b->box_zone_count - 1];
        box_set_cell(world.button_count - 1, cell);
    }
    tick_idle(1);
    assert(world.buttons_pressed == world.button_count);
    assert(world.doors[0].open == 1);

    /* park the dog off the zones (length 0 so no part overlaps anything):
     * unpressing buttons closes nothing, but drops the pressed count */
    for (int j = 0; j < box_count(); j++) {
        if (box_alive(j)) box_set_cell(j, sim_cell_of(0, 0));
    }
    world.dog.alive = 0;
    tick_idle(1);
    assert(world.buttons_pressed == 0);

    /* the head's cell counts as pressing (the original stupidblock) */
    SimButton *last = &world.buttons[0];
    world.dog.alive = 1;
    world.dog.length = 0;
    world.dog.cx = sim_cell_x(last->zone[0]);
    world.dog.cy = sim_cell_y(last->zone[0]);
    tick_idle(1);
    assert(last->pressed == 1);
    assert(world.buttons_pressed == 1);

    /* win: length 2 makes the house ready; stepping into the win zone
     * advances to the next runtime room */
    world.dog.length = 2;
    tick_idle(1);
    assert(house_remain() == 0);
    assert(house_win_alive());
    world.dog.cx = sim_cell_x(house_win_zone_cell(0));
    world.dog.cy = sim_cell_y(house_win_zone_cell(0));
    tick_idle(2);
    assert(!house_win_alive());
    assert(transition_state()->next_lvl);
    tick_idle(200);
    assert(world.room_index == SIM_ROOM_LEVEL3);
    assert(strcmp(world.room->name, "rm_level3") == 0);
    /* room_num counts wins, not room indices */
    assert(transition_state()->room_num == 2);
}

static void test_retry_reloads_room(void)
{
    int room_before = world.room_index;
    SimInput input;
    memset(&input, 0, sizeof(input));
    input.pressed_r = 1;
    tick_with(&input);
    tick_idle(200);
    assert(world.room_index == room_before);
    assert(dog()->alive);
    assert(dog()->length == 5);
    assert(dog()->play == 1); /* the level3 room has no dialogue */
}

int main(void)
{
    test_title_flow_and_room_order();
    test_tutorial_dialogue_gates_play();
    test_movement_and_chain();
    test_apple_and_skull_length();
    test_walls_and_push_rules();
    test_buttons_door_win_retry();
    test_retry_reloads_room();
    printf("longo_game_smoke: all tests passed\n");
    return 0;
}
