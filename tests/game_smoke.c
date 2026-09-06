/*
 * Headless smoke tests for the simulation core.
 * Runs the recovered rooms with deterministic input and asserts the ported
 * rules: runtime room order, dialogue gating, tile-based movement cadence,
 * chain follow, apple/skull length changes, box push/hole fill,
 * simultaneous button/door logic, the win transition order, and retry.
 */
#include "core/world.h"

#include "core/solid.h"
#include "core/view.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "objects/box.h"
#include "objects/dog.h"
#include "objects/button.h"
#include "objects/dialogue.h"
#include "objects/door.h"
#include "objects/hole.h"
#include "objects/house.h"
#include "objects/items.h"
#include "objects/title.h"
#include "objects/transition.h"

#define world (*world_ptr())

static void tick_with(const SimInput *input) { sim_tick(world_ptr(), input); }

static void tick_idle(int n)
{
    SimInput none;
    memset(&none, 0, sizeof(none));
    for (int i = 0; i < n; i++) sim_tick(world_ptr(), &none);
}

static int box_at_cell(int cx, int cy)
{
    return box_index_at(sim_cell_of(cx, cy));
}

static int hole_at_cell(int cx, int cy)
{
    return hole_index_at(sim_cell_of(cx, cy));
}

/* one movement press, spaced past the key_cooldown (alarm[1] = 2) */
static void press_dir(int dir)
{
    SimInput in;
    memset(&in, 0, sizeof(in));
    if (dir == 0) in.pressed_down = 1;
    else if (dir == 90) in.pressed_right = 1;
    else if (dir == 180) in.pressed_up = 1;
    else in.pressed_left = 1;
    tick_with(&in);
    tick_idle(1);
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
    assert(dog_alive());
    assert(dog_length() == 5);

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

    assert(!dog_play()); /* the tutorial paused the dog */
    for (int i = 0; i < 6; i++) {
        input.pressed_space = 1;
        tick_with(&input);
        input.pressed_space = 0;
        tick_idle(1);
    }
    /* index == last: the next press releases the dog */
    assert(!dog_play());
    input.pressed_space = 1;
    tick_with(&input);
    tick_idle(DIALOGUE_SHRINK_TICKS + 10); /* box shrink, then gone */
    assert(dog_play());
    assert(!dialogue_active());
}

static void test_movement_and_chain(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));
    int start_x = dog_cx();
    int start_y = dog_cy();
    uint16_t head0 = sim_cell_of(start_x, start_y);

    /* a fresh press steps instantly and snaps the head cell */
    input.pressed_right = 1;
    tick_with(&input);
    assert(dog_cx() == start_x + 1);
    assert(dog_dir() == 90);

    /* the key_cooldown gate: an immediate re-press is swallowed
     * (alarm[1] = 2 keeps the move locked for the next tick) */
    tick_with(&input);
    assert(dog_cx() == start_x + 1); /* cooldown still counting */

    /* holding the key alone produces nothing further: the original
     * oDog Step moves on keyboard_check_pressed, not held keys */
    tick_idle(5);
    assert(dog_cx() == start_x + 1);

    /* a new press after the cooldown steps again */
    input.pressed_right = 1;
    tick_with(&input);
    assert(dog_cx() == start_x + 2);

    /* the body follows into the head's previous cells */
    tick_idle(3);
    assert(dog_part_cell(0) == sim_cell_of(start_x + 1, start_y));
    assert(dog_part_cell(1) == head0);
    assert(dog_play());
}

static void test_room_load_rules(void)
{
    /* scaled oBlock instances must blanket every cell their scaled bbox
     * covers: the tutorial border is stamped as 19x1 / 1x12.5 blocks */
    sim_room_goto(world_ptr(), SIM_ROOM_TUTORIAL);
    tick_idle(2);

    /* the dialogue bubble is a 9-slice panel, not a scaled sprite */
    view_begin_frame();
    dialogue_draw();
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_GUI, &count);
        int patches = 0;
        for (int i = 0; i < count; i++)
            if (items[i].kind == VIEW_ITEM_NINE_PATCH &&
                items[i].sprite == LONGO_SPR_DIALOGUEBOX)
                patches++;
        assert(patches == 1);
    }
    for (int x = 0; x < world.cells_w; x++) {
        assert(solid_kind_at(sim_cell_of(x, 0)) == SOLID_WALL);      /* 19x1 */
        assert(solid_kind_at(sim_cell_of(x, 12)) == SOLID_WALL);     /* 19x1 */
        assert(solid_kind_at(sim_cell_of(0, x % world.cells_h)) ==
               SOLID_WALL);                                          /* 1x12.5 */
        assert(solid_kind_at(sim_cell_of(18, x % world.cells_h)) ==
               SOLID_WALL);                                          /* 1x12.5 */
    }
    assert(solid_kind_at(sim_cell_of(5, 5)) == SOLID_EMPTY);

    /* dismissing the restarted tutorial dialogue releases the dog */
    {
        SimInput input;
        memset(&input, 0, sizeof(input));
        for (int i = 0; i < 7; i++) {
            input.pressed_space = 1;
            tick_with(&input);
            input.pressed_space = 0;
            tick_idle(1);
        }
        tick_idle(DIALOGUE_SHRINK_TICKS + 10);
    }
    assert(dog_play());

    /* and the dog really cannot step into the stamp */
    dog_teleport(17, 5);
    {
        SimInput input;
        memset(&input, 0, sizeof(input));
        input.pressed_right = 1;
        tick_with(&input);
    }
    assert(dog_strain());
    assert(dog_cx() == 17 && dog_cy() == 5);

    /* the tail part is not solid (block = 0 in the original): wrap a
     * length-3 dog around a 4-cell loop; from the fourth step on, the
     * head steps onto the cell the tail occupies every single move */
    dog_set_length(3);
    dog_teleport(4, 5);
    {
        /* loop (4,5) -> (5,5) -> (5,6) -> (4,6) -> (4,5) ... */
        static const int loop[] = { 90, 0, 270, 180 };
        static const int ex[][2] = { { 5, 5 }, { 5, 6 }, { 4, 6 },
                                     { 4, 5 }, { 5, 5 }, { 5, 6 },
                                     { 4, 6 }, { 4, 5 } };
        for (int i = 0; i < 8; i++) {
            press_dir(loop[i % 4]);
            assert(dog_cx() == ex[i][0] && dog_cy() == ex[i][1]);
            assert(!dog_strain());
        }
    }
    dog_set_length(5);

    /* view state is born on the dog's cell, never eased in from (0,0) */
    assert(dog_visual_x() == (float)(dog_cx() * 16 + 8));
    assert(dog_visual_y() == (float)(dog_cy() * 16 + 8));

    /* oSkull draws sprPear (the recovered object table), one item per
     * pushed view sprite */
    view_begin_frame();
    items_draw(0);
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        int pears = 0;
        for (int i = 0; i < count; i++)
            if (items[i].kind == VIEW_ITEM_SPRITE &&
                items[i].sprite == LONGO_SPR_PEAR)
                pears++;
        assert(pears > 0);
        assert(pears == skull_count());
    }

    /* the house counter keeps the recovered digits font (font_id 2) */
    view_begin_frame();
    house_draw(0);
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        int digits_items = 0;
        for (int i = 0; i < count; i++)
            if (items[i].kind == VIEW_ITEM_TEXT && items[i].font_id == 2)
                digits_items++;
        assert(digits_items == 2); /* shadow + main */
    }

    /* the house anchors on the oGoal instance (bottom-centre, sprHouse
     * origin (32,64)) and oGoalUp redraws the top 44 rows above it */
    view_begin_frame();
    house_draw(0);
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        uint16_t goal = house_goal_cell();
        float gx = (float)(sim_cell_x(goal) * 16 + 8);
        float gy = (float)(sim_cell_y(goal) * 16 + 32);
        int base = 0, crop = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].sprite != LONGO_SPR_HOUSE) continue;
            if (items[i].kind == VIEW_ITEM_SPRITE) {
                base++;
                assert(items[i].x == gx && items[i].y == gy);
            } else if (items[i].kind == VIEW_ITEM_SPRITE_PART) {
                crop++;
                assert(items[i].x == gx - 32.0f);
                assert(items[i].y == gy - 64.0f);
            }
        }
        assert(base == 1);
        assert(crop == 1);
    }

    /* level1 places the house through oHouseSpawner and has boxes: the
     * spawned goal anchors at (x+8, y+16) and box views start on-cell */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    assert(box_count() > 0);
    for (int i = 0; i < box_count(); i++) {
        assert(box_visual_x(i) == (float)(sim_cell_x(box_cell(i)) * 16));
        assert(box_visual_y(i) == (float)(sim_cell_y(box_cell(i)) * 16));
    }
    view_begin_frame();
    house_draw(0);
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        /* spawner at (64,48): goal instance at (72,64) */
        for (int i = 0; i < count; i++) {
            if (items[i].sprite != LONGO_SPR_HOUSE) continue;
            if (items[i].kind == VIEW_ITEM_SPRITE)
                assert(items[i].x == 72.0f && items[i].y == 64.0f);
            else if (items[i].kind == VIEW_ITEM_SPRITE_PART)
                assert(items[i].x == 40.0f && items[i].y == 0.0f);
        }
    }
}

static void test_apple_and_skull_length(void)
{
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
    dog_teleport(sim_cell_x(apple_cell(apple)) - 1,
                 sim_cell_y(apple_cell(apple)));
    int length_before = dog_length();

    input.pressed_right = 1;
    tick_with(&input);
    tick_idle(1);
    assert(dog_length() == length_before + 1);
    assert(!apple_alive(apple));
    /* the new tail segment sits on the cell the tail just left */
    assert(dog_part_cell(dog_length() - 1) == dog_detached_cell());

    /* a skull at length 3 shortens; at length 2 it kills the dog */
    int skull = -1;
    for (int i = 0; i < skull_count(); i++) {
        if (skull_alive(i)) {
            skull = i;
            break;
        }
    }
    assert(skull >= 0);
    dog_set_length(3);
    dog_teleport(sim_cell_x(skull_cell(skull)) - 1,
                 sim_cell_y(skull_cell(skull)));
    input.pressed_right = 1;
    tick_with(&input);
    tick_idle(1);
    assert(dog_alive());
    assert(dog_length() == 2);

    for (int i = 0; i < skull_count(); i++) {
        if (skull_alive(i)) {
            skull = i;
            break;
        }
    }
    assert(skull >= 0);
    dog_teleport(sim_cell_x(skull_cell(skull)) - 1,
                 sim_cell_y(skull_cell(skull)));
    input.pressed_right = 1;
    tick_with(&input);
    tick_idle(1);
    /* eating a pear at length 2 destroys the dog (original behaviour) */
    assert(!dog_alive());
}

static void test_walls_and_push_rules(void)
{
    /* rm_level1 (runtime index SIM_ROOM_LEVEL1) exercises boxes, holes,
     * buttons, doors */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    assert(strcmp(world.room->name, "rm_level1") == 0);
    tick_idle(2);

    int box = box_at_cell(6, 5);
    int hole = hole_at_cell(13, 5);
    assert(box >= 0);
    assert(hole >= 0);
    assert(!hole_is_full(hole));

    /* a box pushed into the hole fills it and the box is destroyed */
    box_set_cell(box, sim_cell_of(12, 5)); /* one cell left of the hole */
    {
        SimInput input;
        memset(&input, 0, sizeof(input));
        dog_teleport(11, 5);
        input.pressed_right = 1;
        tick_with(&input);
    }
    assert(hole_is_full(hole));
    assert(!box_alive(box));
    assert(dog_cx() == 12); /* the dog took the box's old cell */

    /* a second box pushed onto the filled hole rides on top of it like
     * normal ground instead of being swallowed again */
    {
        SimInput input;
        memset(&input, 0, sizeof(input));
        dog_teleport(11, 5);
        assert(box_at_cell(12, 5) < 0);
        assert(box_alive(1));
        box_set_cell(1, sim_cell_of(12, 5));
        input.pressed_right = 1;
        tick_with(&input);
        int rider = box_at_cell(13, 5);
        assert(rider >= 0 && box_alive(rider));
        assert(hole_is_full(hole)); /* still just filled, not re-filled */
        assert(dog_cx() == 12);
        box_set_cell(rider, sim_cell_of(0, 0));
        tick_idle(1);
    }

    /* holes render via oHole's draw event; the filled one shows frame 1 */
    view_begin_frame();
    hole_draw();
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        int holes_seen = 0, filled = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].kind != VIEW_ITEM_SPRITE ||
                items[i].sprite != LONGO_SPR_HOLE)
                continue;
            holes_seen++;
            if (items[i].frame == 1) filled++;
        }
        assert(holes_seen == hole_count());
        assert(filled == 1);
    }

    /* walls block: stepping into a static block strains in place */
    SimInput input;
    memset(&input, 0, sizeof(input));
    int cx = -1, cy = -1;
    for (int y = 0; y < world.cells_h && cx < 0; y++) {
        for (int x = 1; x < world.cells_w && cx < 0; x++) {
            uint16_t here = sim_cell_of(x, y);
            uint16_t left = sim_cell_of(x - 1, y);
            if (solid_kind_at(here) == SOLID_WALL &&
                solid_kind_at(left) == SOLID_EMPTY) {
                cx = x - 1;
                cy = y;
            }
        }
    }
    assert(cx >= 0);
    dog_teleport(cx, cy);
    input.pressed_right = 1;
    tick_with(&input);
    tick_idle(4);
    assert(dog_strain());
    assert(dog_cx() == cx && dog_cy() == cy);
}

static void test_buttons_door_win_retry(void)
{
    /* rm_level1 has four buttons; doors open once all are pressed at once */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    assert(!door_open(0));

    /* a box on the cell below a button must not press it: the original's
     * 4px lid overlap read as a false press and was dropped */
    {
        uint16_t own = button_zone_cell(0, 0);
        uint16_t below = sim_cell_of(sim_cell_x(own), sim_cell_y(own) + 1);
        assert(box_index_at(own) < 0);
        box_set_cell(0, below);
        tick_idle(1);
        assert(!button_pressed(0));
        box_set_cell(0, sim_cell_of(0, 0));
        tick_idle(1);
    }

    /* unpressed buttons cycle sprButton's 9 frames on the shared clock */
    view_begin_frame();
    button_draw(0);
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        float clock = view_sprite_clock(LONGO_SPR_BUTTON);
        int animated = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].kind != VIEW_ITEM_SPRITE ||
                items[i].sprite != LONGO_SPR_BUTTON)
                continue;
            assert(items[i].frame == (int)clock % 9);
            animated++;
        }
        assert(animated == button_count());
    }

    /* park a distinct box on every button zone but the last one */
    for (int i = 0; i < button_count() - 1; i++) {
        uint16_t cell = button_zone_cell(i, 0);
        int existing = box_index_at(cell);
        if (existing >= 0 && existing != i)
            box_set_cell(existing, sim_cell_of(0, 0));
        assert(i < box_count());
        box_set_cell(i, cell);
        tick_idle(1);
    }
    tick_idle(1);
    {
        int pressed = 0;
        for (int i = 0; i < button_count() - 1; i++)
            if (button_pressed(i)) pressed++;
        assert(pressed == button_count() - 1);
    }
    /* doors stay closed until every button is pressed simultaneously */
    assert(!door_open(0));

    /* the last button: a box on its cell opens the doors */
    box_set_cell(button_count() - 1, button_zone_cell(button_count() - 1, 0));
    tick_idle(1);
    assert(door_open(0));

    /* park the dog off the zones and clear the boxes */
    for (int j = 0; j < box_count(); j++) {
        if (box_alive(j)) box_set_cell(j, sim_cell_of(0, 0));
    }
    dog_set_alive(false);
    tick_idle(1);

    /* the head's cell counts as pressing (the original stupidblock) */
    dog_set_alive(true);
    dog_set_length(0);
    dog_teleport(sim_cell_x(button_zone_cell(0, 0)),
                 sim_cell_y(button_zone_cell(0, 0)));
    tick_idle(1);
    assert(button_pressed(0));

    /* win: length 2 makes the house ready; stepping into the win zone
     * advances to the next runtime room */
    dog_set_length(2);
    tick_idle(1);
    assert(house_remain() == 0);
    assert(house_win_alive());
    dog_teleport(sim_cell_x(house_win_zone_cell(0)),
                 sim_cell_y(house_win_zone_cell(0)));
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
    assert(dog_alive());
    assert(dog_length() == 5);
    assert(dog_play()); /* the level3 room has no dialogue */
}

int main(void)
{
    test_title_flow_and_room_order();
    test_tutorial_dialogue_gates_play();
    test_movement_and_chain();
    test_room_load_rules();
    test_apple_and_skull_length();
    test_walls_and_push_rules();
    test_buttons_door_win_retry();
    test_retry_reloads_room();
    printf("longo_game_smoke: all tests passed\n");
    return 0;
}
