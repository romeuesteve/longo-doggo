/*
 * Headless smoke tests for the simulation core.
 * Runs every room with deterministic input and asserts the gameplay
 * rules: runtime room order, dialogue gating, tile-based movement cadence,
 * chain follow, apple/pear length changes, box push/hole fill,
 * simultaneous button/door logic, the win transition order, and retry.
 */

/* Checks must survive optimized builds: Release's -DNDEBUG would
 * compile every assert away, leaving CI with crash detection only.
 * Re-including <assert.h> with NDEBUG undefined keeps the checks live
 * in every configuration. */
#undef NDEBUG
#include <assert.h>

#include "core/world.h"

#include "core/events.h"
#include "core/solid.h"
#include "core/view.h"
#include "room_tiles.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "objects/box.h"
#include "objects/dog.h"
#include "objects/button.h"
#include "objects/dialogue.h"
#include "objects/door.h"
#include "objects/fx.h"
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

/* one movement press, spaced past the key_cooldown (it lasts 2 ticks) */
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

/* Fresh game from the title, straight into the tutorial room with its
 * dialogue released: the state most gameplay scenarios need.  Scenarios
 * must be self-contained so ctest can also run each one in its own
 * process. */
static void start_playable_in_tutorial(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));
    sim_init(42u);
    input.pressed_any = 1;
    tick_with(&input);
    tick_idle(200); /* ride the wipe into the tutorial room */
    for (int i = 0; i < 7; i++) {
        input.pressed_space = 1;
        tick_with(&input);
        input.pressed_space = 0;
        tick_idle(1);
    }
    tick_idle(DIALOGUE_SHRINK_TICKS + 10);
    assert(dog_play());
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
    assert(transition_state()->room_num == 1); /* label counts from 1 */
    assert(dog_alive());
    assert(dog_length() == 5);

    input.pressed_any = 1;
    tick_with(&input);
    assert(transition_state()->phase == TRANSITION_OPENING); /* the title
                                                                starts the wipe */
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

    sim_init(42u);
    input.pressed_any = 1;
    tick_with(&input);
    tick_idle(200); /* ride the wipe into the tutorial room */

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
    start_playable_in_tutorial();
    int start_x = dog_cx();
    int start_y = dog_cy();
    uint16_t head0 = sim_cell_of(start_x, start_y);

    /* a fresh press steps instantly and snaps the head cell */
    input.pressed_right = 1;
    tick_with(&input);
    assert(dog_cx() == start_x + 1);
    assert(dog_dir() == 90);

    /* the key_cooldown gate: an immediate re-press is swallowed
     * (the cooldown keeps the move locked for the next tick) */
    tick_with(&input);
    assert(dog_cx() == start_x + 1); /* cooldown still counting */

    /* holding the key alone produces nothing further: movement is
     * edge-triggered, one step per press, not held keys */
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
    /* scaled wall stamps must blanket every cell their scaled footprint
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

    /* the tail part is not solid: wrap a
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

    /* the pear item draws the pear sprite, one item per pushed view
     * sprite */
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
        assert(pears == pear_count());
    }

    /* the house counter keeps the pixel digits font (font_id 2) */
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

    /* the house anchors bottom-centre (sprHouse origin (32,64)) and the
     * squash pass redraws the top 44 rows above it */
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

    /* level1 places the house through the house spawner and has boxes:
     * the spawned goal anchors at (x+8, y+16) and box views start on-cell */
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
        /* spawner at (64,48): goal at (72,64) */
        for (int i = 0; i < count; i++) {
            if (items[i].sprite != LONGO_SPR_HOUSE) continue;
            if (items[i].kind == VIEW_ITEM_SPRITE)
                assert(items[i].x == 72.0f && items[i].y == 64.0f);
            else if (items[i].kind == VIEW_ITEM_SPRITE_PART)
                assert(items[i].x == 40.0f && items[i].y == 0.0f);
        }
    }
}

static void test_apple_and_pear_length(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));

    start_playable_in_tutorial();

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

    /* a pear at length 3 shortens the dog to the minimum shape */
    int pear = -1;
    for (int i = 0; i < pear_count(); i++) {
        if (pear_alive(i)) {
            pear = i;
            break;
        }
    }
    assert(pear >= 0);
    dog_set_length(3);
    dog_teleport(sim_cell_x(pear_cell(pear)) - 1,
                 sim_cell_y(pear_cell(pear)));
    input.pressed_right = 1;
    tick_with(&input);
    tick_idle(1);
    assert(dog_length() == 2);

    /* at the minimum length a pear is still eaten, but nothing
     * shrinks and the dog survives */
    for (int i = 0; i < pear_count(); i++) {
        if (pear_alive(i)) {
            pear = i;
            break;
        }
    }
    assert(pear >= 0);
    dog_teleport(sim_cell_x(pear_cell(pear)) - 1,
                 sim_cell_y(pear_cell(pear)));
    input.pressed_right = 1;
    tick_with(&input);
    tick_idle(1);
    assert(dog_length() == 2);
    assert(!pear_alive(pear));
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

    /* holes render via the hole sprite; the filled one shows frame 1 */
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
    start_playable_in_tutorial();
    /* rm_level1 has four buttons; doors open once all are pressed at once */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    assert(!door_open(0));

    /* a box on the cell below a button must not press it: its visual
     * overlap would read as a false press */
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

    /* the head's own cell counts as pressing */
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
    assert(transition_state()->pending == TRANSITION_ACTION_NEXT_ROOM);
    tick_idle(200);
    assert(world.room_index == SIM_ROOM_LEVEL3);
    assert(strcmp(world.room->name, "rm_level3") == 0);
    /* room_num counts wins, not room indices */
    assert(transition_state()->room_num == 2);
}

static void test_retry_reloads_room(void)
{
    start_playable_in_tutorial();
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    int room_before = world.room_index;
    SimInput input;
    memset(&input, 0, sizeof(input));
    input.pressed_r = 1;
    tick_with(&input);
    tick_idle(200);
    assert(world.room_index == room_before);
    assert(dog_alive());
    assert(dog_length() == 5);
    assert(dog_play()); /* the room has no dialogue, so retry leaves
                           the dog playable */
}

/* Room-flow policy lives in sim_tick(): R requests a retry, a closing
 * wipe's pending room change beats R, and mashing R mid-wipe still
 * queues exactly one reload. */
static void test_room_flow_requests(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));

    /* (a) R reloads the current room: a moved box snaps back and the
     * dog respawns fresh */
    start_playable_in_tutorial();
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    box_set_cell(0, sim_cell_of(2, 2)); /* mark the box moved */
    tick_idle(1);
    input.pressed_r = 1;
    tick_with(&input);
    tick_idle(200);
    assert(world.room_index == SIM_ROOM_LEVEL1);
    assert(box_cell(0) == sim_cell_of(6, 5)); /* back at its spawn */
    assert(dog_alive());
    assert(dog_length() == 5);
    assert(dog_play()); /* no dialogue here: retry keeps the dog playable */

    /* (b) win into the house, then mash R during the closing wipe: the
     * pending next-room action must not be hijacked into a retry */
    for (int i = 0; i < button_count(); i++)
        box_set_cell(i, button_zone_cell(i, 0));
    tick_idle(1);
    assert(door_open(0));
    for (int j = 0; j < box_count(); j++) {
        if (box_alive(j)) box_set_cell(j, sim_cell_of(0, 0));
    }
    dog_set_alive(false);
    tick_idle(1);
    dog_set_alive(true);
    dog_set_length(0);
    dog_teleport(sim_cell_x(button_zone_cell(0, 0)),
                 sim_cell_y(button_zone_cell(0, 0)));
    tick_idle(1);
    dog_set_length(2);
    tick_idle(1);
    assert(house_remain() == 0);
    assert(house_win_alive());
    dog_teleport(sim_cell_x(house_win_zone_cell(0)),
                 sim_cell_y(house_win_zone_cell(0)));
    tick_idle(2);
    assert(!house_win_alive());
    assert(transition_state()->pending == TRANSITION_ACTION_NEXT_ROOM);
    {
        int guard = 0;
        while (!transition_closing() && guard < 400) {
            tick_idle(1);
            guard++;
        }
        assert(transition_closing());
        while (transition_closing() && guard < 800) {
            input.pressed_r = 1;
            tick_with(&input);
            input.pressed_r = 0;
            guard++;
        }
        /* the R presses during the closing wipe were gated away */
        assert(transition_state()->phase == TRANSITION_IDLE);
    }
    assert(world.room_index == SIM_ROOM_LEVEL3);
    assert(strcmp(world.room->name, "rm_level3") == 0);
    tick_idle(30);
    assert(transition_state()->phase == TRANSITION_IDLE); /* no retry wipe
                                                             followed */
    assert(world.room_index == SIM_ROOM_LEVEL3);
    assert(dog_alive());
    assert(dog_length() == 5);

    /* (c) R pressed on every tick of the wipe: exactly one reload, the
     * dog stays sane */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    input.pressed_r = 1;
    tick_with(&input);
    {
        int guard = 0;
        while (transition_state()->phase == TRANSITION_OPENING &&
               guard < 400) {
            tick_with(&input); /* a fresh R edge every tick */
            guard++;
        }
        assert(transition_closing()); /* the wipe ran, reload included */
    }
    tick_idle(200);
    assert(world.room_index == SIM_ROOM_LEVEL1);
    assert(dog_alive());
    assert(dog_length() == 5);
    {
        long loaded = world.room_loaded_tick;
        tick_idle(60);
        assert(world.room_loaded_tick == loaded); /* no second reload */
        assert(transition_state()->phase == TRANSITION_IDLE);
    }

    /* (d) R during the OPENING phase of a win wipe: the retry replaces
     * the pending advance and reverses the win increment, so the same
     * room reloads with its pre-win label */
    memset(&input, 0, sizeof(input));
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    {
        int room_num_before = transition_state()->room_num;
        for (int i = 0; i < button_count(); i++)
            box_set_cell(i, button_zone_cell(i, 0));
        tick_idle(1);
        assert(door_open(0));
        for (int j = 0; j < box_count(); j++) {
            if (box_alive(j)) box_set_cell(j, sim_cell_of(0, 0));
        }
        dog_set_alive(false);
        tick_idle(1);
        dog_set_alive(true);
        dog_set_length(0);
        dog_teleport(sim_cell_x(button_zone_cell(0, 0)),
                     sim_cell_y(button_zone_cell(0, 0)));
        tick_idle(1);
        dog_set_length(2);
        tick_idle(1);
        assert(house_remain() == 0);
        assert(house_win_alive());
        dog_teleport(sim_cell_x(house_win_zone_cell(0)),
                     sim_cell_y(house_win_zone_cell(0)));
        tick_idle(2);
        assert(!house_win_alive());
        assert(transition_state()->pending == TRANSITION_ACTION_NEXT_ROOM);
        assert(transition_state()->room_num == room_num_before + 1);
        input.pressed_r = 1; /* the first opening tick: before midpoint */
        tick_with(&input);
        input.pressed_r = 0;
        assert(transition_state()->pending == TRANSITION_ACTION_RETRY);
        tick_idle(200); /* ride the wipes to completion */
        assert(world.room_index == SIM_ROOM_LEVEL1); /* retry semantics */
        assert(strcmp(world.room->name, "rm_level1") == 0);
        assert(transition_state()->room_num == room_num_before);
        assert(transition_state()->pending == TRANSITION_ACTION_NONE);
        assert(transition_state()->phase == TRANSITION_IDLE);
        assert(dog_alive());
        assert(dog_length() == 5);
    }

    /* (e) the mirror case: a win arriving while a retry wipe is pending
     * keeps its increment and supersedes the retry */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    {
        int room_num_before = transition_state()->room_num;
        input.pressed_r = 1; /* retry requested before any win */
        tick_with(&input);
        input.pressed_r = 0;
        assert(transition_state()->pending == TRANSITION_ACTION_RETRY);
        /* the win setup runs inside the opening wipe, before midpoint */
        for (int i = 0; i < button_count(); i++)
            box_set_cell(i, button_zone_cell(i, 0));
        tick_idle(1);
        assert(door_open(0));
        for (int j = 0; j < box_count(); j++) {
            if (box_alive(j)) box_set_cell(j, sim_cell_of(0, 0));
        }
        dog_set_alive(false);
        tick_idle(1);
        dog_set_alive(true);
        dog_set_length(0);
        dog_teleport(sim_cell_x(button_zone_cell(0, 0)),
                     sim_cell_y(button_zone_cell(0, 0)));
        tick_idle(1);
        dog_set_length(2);
        tick_idle(1);
        assert(house_remain() == 0);
        assert(house_win_alive());
        dog_teleport(sim_cell_x(house_win_zone_cell(0)),
                     sim_cell_y(house_win_zone_cell(0)));
        tick_idle(2);
        assert(!house_win_alive());
        assert(transition_state()->phase == TRANSITION_OPENING);
        assert(transition_state()->pending == TRANSITION_ACTION_NEXT_ROOM);
        assert(transition_state()->room_num == room_num_before + 1);
        tick_idle(300); /* ride it out: an advance, not the retry */
        assert(world.room_index == SIM_ROOM_LEVEL3);
        assert(strcmp(world.room->name, "rm_level3") == 0);
        assert(transition_state()->room_num == room_num_before + 1);
        assert(transition_state()->pending == TRANSITION_ACTION_NONE);
        assert(transition_state()->phase == TRANSITION_IDLE);
    }
}

/* one undo press, spaced like a movement press */
static void press_undo(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));
    input.pressed_undo = 1;
    tick_with(&input);
    tick_idle(1);
}

static void test_fx_pools_reuse_dead_slots(void)
{
    fx_reset();

    /* Fill the popup and sink pools to their caps, let every slot die,
     * then fill them again.  The pools used to be append-only: death
     * never shrank the counts, so the whole second wave was silently
     * dropped after a few minutes of play. */
    for (int i = 0; i < 16; i++)
        events_fx(FX_ONE, 0.0f, (float)i, 0, 0, 0, 0, 0);
    for (int i = 0; i < 8; i++)
        events_fx(FX_BOX_SINK, 0.0f, (float)(16 + i), 640.0f, 480.0f, 0, 0,
                  0);
    fx_tick();
    view_begin_frame();
    fx_draw();
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        int ones = 0, boxes = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].kind != VIEW_ITEM_SPRITE) continue;
            if (items[i].sprite == LONGO_SPR_ONE) ones++;
            if (items[i].sprite == LONGO_SPR_BOX) boxes++;
        }
        assert(ones == 16);
        assert(boxes == 8);
    }

    /* popups fade in 45 ticks, sinks arrive in ~25: nothing survives
     * 60 */
    for (int t = 0; t < 60; t++) fx_tick();
    view_begin_frame();
    fx_draw();
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        int ones = 0, boxes = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].kind != VIEW_ITEM_SPRITE) continue;
            if (items[i].sprite == LONGO_SPR_ONE) ones++;
            if (items[i].sprite == LONGO_SPR_BOX) boxes++;
        }
        assert(ones == 0);
        assert(boxes == 0);
    }

    /* second wave: every spawn must land in a recycled slot */
    for (int i = 0; i < 16; i++)
        events_fx(FX_ONE, 0.0f, (float)i, 0, 0, 0, 0, 0);
    for (int i = 0; i < 8; i++)
        events_fx(FX_BOX_SINK, 0.0f, (float)(16 + i), 640.0f, 480.0f, 0, 0,
                  0);
    fx_tick();
    view_begin_frame();
    fx_draw();
    {
        int count = 0;
        const ViewItem *items = view_items(VIEW_WORLD, &count);
        int ones = 0, boxes = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].kind != VIEW_ITEM_SPRITE) continue;
            if (items[i].sprite == LONGO_SPR_ONE) ones++;
            if (items[i].sprite == LONGO_SPR_BOX) boxes++;
        }
        assert(ones == 16);
        assert(boxes == 8);
    }
}

static void test_undo(void)
{
    int start_x, start_y;

    /* empty history: an undo press is a harmless no-op */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    start_x = dog_cx();
    start_y = dog_cy();
    press_undo();
    assert(dog_cx() == start_x && dog_cy() == start_y);

    /* one step, one undo: head cell, facing and chain all return, and
     * the solid map follows (the stepped-into cell is free again) */
    press_dir(90);
    assert(dog_cx() == start_x + 1);
    assert(dog_dir() == 90);

    /* the undo press proper */
    press_undo();
    assert(dog_cx() == start_x && dog_cy() == start_y);
    assert(dog_dir() == 180);
    assert(dog_part_cell(0) == sim_cell_of(start_x, start_y + 1));
    assert(dog_length() == 5);
    assert(solid_kind_at(sim_cell_of(start_x + 1, start_y)) == SOLID_EMPTY);

    /* stepping backwards — the press opposite to the facing, which
     * could only ever strain into the dog's own neck — undoes too */
    press_dir(90);
    assert(dog_cx() == start_x + 1 && dog_dir() == 90);
    press_dir(270);
    assert(dog_cx() == start_x && dog_cy() == start_y);
    assert(dog_dir() == 180);
    assert(solid_kind_at(sim_cell_of(start_x + 1, start_y)) == SOLID_EMPTY);

    /* a box pushed into the hole, then undone: the box is back, the
     * hole is open again and the dog stands where it pushed from */
    {
        int box = box_at_cell(6, 5);
        int hole = hole_at_cell(13, 5);
        SimInput input;
        memset(&input, 0, sizeof(input));
        assert(box >= 0 && hole >= 0 && !hole_is_full(hole));
        box_set_cell(box, sim_cell_of(12, 5));
        dog_teleport(11, 5);
        input.pressed_right = 1;
        tick_with(&input);
        tick_idle(1);
        assert(hole_is_full(hole));
        assert(!box_alive(box));
        press_undo();
        assert(!hole_is_full(hole));
        assert(box_alive(box));
        assert(box_cell(box) == sim_cell_of(12, 5));
        assert(box_index_at(sim_cell_of(12, 5)) == box);
        assert(dog_cx() == 11 && dog_cy() == 5);
    }

    /* eating an apple grows the dog; undo shrinks it back and the
     * apple returns */
    sim_room_goto(world_ptr(), SIM_ROOM_TUTORIAL);
    tick_idle(2);
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
    {
        int apple = -1;
        int length_before;
        SimInput input;
        memset(&input, 0, sizeof(input));
        for (int i = 0; i < apple_count(); i++)
            if (apple_alive(i)) {
                apple = i;
                break;
            }
        assert(apple >= 0);
        length_before = dog_length();
        dog_teleport(sim_cell_x(apple_cell(apple)) - 1,
                     sim_cell_y(apple_cell(apple)));
        input.pressed_right = 1;
        tick_with(&input);
        tick_idle(1);
        assert(dog_length() == length_before + 1);
        assert(!apple_alive(apple));
        press_undo();
        assert(dog_length() == length_before);
        assert(apple_alive(apple));
        assert(dog_cx() == sim_cell_x(apple_cell(apple)) - 1);
    }

    /* a room load drops the history: a fresh room's undo does nothing,
     * not even one left over from the previous room */
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    start_x = dog_cx();
    start_y = dog_cy();
    press_undo();
    assert(dog_cx() == start_x && dog_cy() == start_y);
}

/* A door stays solid through its whole 14-tick open animation and only
 * frees its cell when it poofs. */
static void test_door_open_window_solidity(void)
{
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);

    /* open the door by parking a box on every button */
    for (int i = 0; i < button_count(); i++)
        box_set_cell(i, button_zone_cell(i, 0));
    tick_idle(1);
    assert(door_open(0));
    assert(door_alive(0));

    uint16_t door_cell = 0;
    int found_door = 0;
    for (int cy = 0; cy < world.cells_h; cy++)
        for (int cx = 0; cx < world.cells_w; cx++)
            if (solid_kind_at(sim_cell_of(cx, cy)) == SOLID_DOOR) {
                door_cell = sim_cell_of(cx, cy);
                found_door = 1;
            }
    assert(found_door);

    /* ticks 1..12 of the open window: the door is mid-animation but
     * still blocks its cell (the timer decrements on the opening tick
     * itself, so the poof lands on the 13th) */
    for (int t = 1; t <= 12; t++) {
        tick_idle(1);
        assert(door_alive(0));
        assert(solid_kind_at(door_cell) == SOLID_DOOR);
    }
    tick_idle(1);
    assert(!door_alive(0));
    assert(solid_kind_at(door_cell) == SOLID_EMPTY);
}

/* ---------------------------------------------------------------- */
/* Occupancy invariant: the solid map mirrors the live entities.     */

static unsigned char pinned_kind[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];

/* Walls, the goal mask and holes never move or die: pin them once per
 * room load so the checker only rebuilds the movers. */
static void pin_static_occupancy(void)
{
    for (int cy = 0; cy < SIM_MAX_CELLS_H; cy++) {
        for (int cx = 0; cx < SIM_MAX_CELLS_W; cx++) {
            uint16_t cell = sim_cell_of(cx, cy);
            SolidKind kind = solid_kind_at(cell);
            pinned_kind[cell] =
                (unsigned char)((kind == SOLID_WALL || kind == SOLID_GOAL ||
                                 kind == SOLID_HOLE)
                                    ? kind
                                    : SOLID_EMPTY);
        }
    }
}

/* Rebuild the expected map from the live entity accessors and compare
 * it cell by cell: every stamped cell belongs to a live entity and no
 * live entity cell lacks its stamp.  A box may rest on the dog's
 * non-solid tail cell, so boxes overlay the dog (the map keeps one
 * kind per cell; the stamp order matches the sim's). */
static void assert_occupancy_matches_entities(void)
{
    unsigned char expect[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];
    memcpy(expect, pinned_kind, sizeof(expect));

    if (dog_alive()) {
        expect[sim_cell_of(dog_cx(), dog_cy())] = SOLID_HEAD;
        for (int i = 0; i < dog_length(); i++)
            expect[dog_part_cell(i)] = SOLID_BODY;
    }
    for (int i = 0; i < box_count(); i++) {
        if (box_alive(i)) expect[box_cell(i)] = SOLID_BOX;
    }
    for (int i = 0; i < DOOR_MAX; i++) {
        if (door_alive(i)) expect[door_cell(i)] = SOLID_DOOR;
    }

    for (int cy = 0; cy < SIM_MAX_CELLS_H; cy++) {
        for (int cx = 0; cx < SIM_MAX_CELLS_W; cx++) {
            uint16_t cell = sim_cell_of(cx, cy);
            if (solid_kind_at(cell) != (SolidKind)expect[cell]) {
                fprintf(stderr, "occupancy: cell (%d,%d) map=%d expected=%d\n",
                        cx, cy, (int)solid_kind_at(cell), expect[cell]);
                assert(solid_kind_at(cell) == (SolidKind)expect[cell]);
            }
        }
    }
}

/* Every shipped room loads with the map exactly mirroring its entities
 * (the title S-curve must not leave the spawn footprint behind), and
 * every footprint mutation — box pushes including onto the tail cell,
 * pear shrink, alive toggles, length changes, door removal — keeps the
 * map in step. */
static void test_occupancy_matches_entities(void)
{
    for (int room = 0; room < longo_room_play_count(); room++) {
        sim_room_goto(world_ptr(), room);
        tick_idle(2);
        pin_static_occupancy();
        if (room == SIM_ROOM_TITLE) {
            /* regression: the title room's dog spawn is cell (3,4) and
             * the S-curve restamp must not leave it stamped */
            assert(solid_kind_at(sim_cell_of(3, 4)) == SOLID_EMPTY);
            assert(solid_kind_at(sim_cell_of(10, 10)) == SOLID_HEAD);
        }
        assert_occupancy_matches_entities();
    }

    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL1);
    tick_idle(2);
    pin_static_occupancy();

    /* a box pushed into the hole dies: the map keeps the (now filled)
     * hole and the dog's own footprint, nothing of the box */
    {
        int hole = hole_at_cell(15, 10);
        SimInput input;
        memset(&input, 0, sizeof(input));
        assert(hole >= 0 && !hole_is_full(hole));
        assert(box_at_cell(16, 10) == 3); /* box 3 spawns beside the hole */
        dog_set_length(2);
        dog_teleport(17, 10);
        input.pressed_left = 1;
        tick_with(&input);
        tick_idle(1);
        assert(!box_alive(3));
        assert(hole_is_full(hole));
        assert_occupancy_matches_entities();
    }

    /* a push onto the tail cell: the vacated cell keeps the box's
     * stamp (the tail is not solid, so the box may rest there) */
    {
        assert(box_alive(1));
        dog_set_length(3);
        dog_teleport(8, 5);
        press_dir(90); /* head (9,5); tail now at (8,7) */
        box_set_cell(1, sim_cell_of(8, 7));
        assert_occupancy_matches_entities();
        press_dir(90); /* the tail vacates around the resting box */
        assert(box_index_at(sim_cell_of(8, 7)) == 1);
        assert(solid_kind_at(sim_cell_of(8, 7)) == SOLID_BOX);
        assert_occupancy_matches_entities();
    }

    /* test hooks keep the map exact too */
    dog_set_length(5);
    assert_occupancy_matches_entities();
    dog_set_length(2);
    assert_occupancy_matches_entities();
    dog_set_alive(false);
    assert_occupancy_matches_entities();
    dog_set_alive(true);
    assert_occupancy_matches_entities();

    /* a door stays stamped through its open animation and frees its
     * cell when it poofs (every alive box parked on a button; box 3
     * died in the hole above) */
    {
        int used = 0;
        for (int b = 0; b < box_count() && used < button_count(); b++) {
            if (box_alive(b)) box_set_cell(b, button_zone_cell(used++, 0));
        }
        assert(used == button_count());
    }
    tick_idle(1);
    assert(door_open(0));
    assert_occupancy_matches_entities();
    tick_idle(15);
    assert(!door_alive(0));
    assert_occupancy_matches_entities();

    /* a pear shrinks the chain: the removed tail cell frees in the
     * same operation (the tutorial's open pear at (5,9)) */
    sim_room_goto(world_ptr(), SIM_ROOM_TUTORIAL);
    tick_idle(2);
    pin_static_occupancy();
    {
        int pear = -1;
        int length_before;
        SimInput input;
        memset(&input, 0, sizeof(input));
        for (int i = 0; i < 7; i++) { /* dismiss the tutorial dialogue */
            input.pressed_space = 1;
            tick_with(&input);
            input.pressed_space = 0;
            tick_idle(1);
        }
        tick_idle(DIALOGUE_SHRINK_TICKS + 10);
        assert(dog_play());
        /* walk to the pear at (5,9) from the left, so the length-3
         * chain stays clear of walls, holes and the goal mask */
        dog_set_length(3);
        dog_teleport(2, 7);
        assert_occupancy_matches_entities();
        for (int i = 0; i < pear_count(); i++) {
            if (pear_alive(i) && pear_cell(i) == sim_cell_of(5, 9)) pear = i;
        }
        assert(pear >= 0);
        length_before = dog_length();
        press_dir(90); /* (3,7) */
        press_dir(90); /* (4,7) */
        press_dir(90); /* (5,7) */
        press_dir(0);  /* (5,8) */
        press_dir(0);  /* onto the pear at (5,9) */
        assert(dog_cx() == 5 && dog_cy() == 9);
        assert(dog_length() == length_before - 1);
        assert(!pear_alive(pear));
        assert_occupancy_matches_entities();
    }
}

/* ---------------------------------------------------------------- */
/* New-game determinism: sim_init(seed) is the one complete reset.    */

typedef struct ViewStreamSnap {
    int count;
    ViewItem items[VIEW_MAX_ITEMS];
    int order[VIEW_MAX_ITEMS];
} ViewStreamSnap;

typedef struct EntityCell {
    int alive;
    uint16_t cell;
} EntityCell;

typedef struct NewGameSnapshot {
    /* SimWorld scalars */
    long tick, room_loaded_tick;
    int room_index, cells_w, cells_h, shadows_present;
    unsigned int rng;
    /* dog state via accessors */
    int dog_alive, dog_play, dog_cx, dog_cy, dog_dir, dog_length;
    int dog_strain;
    float dog_vx, dog_vy;
    uint16_t dog_parts[DOG_MAX_CHAIN];
    uint16_t dog_detached;
    /* entity readback */
    int box_cnt;
    EntityCell box[BOX_MAX];
    struct {
        int open;
        EntityCell e;
    } door[DOOR_MAX];
    int apple_cnt, pear_cnt;
    EntityCell apple[ITEMS_MAX], pear[ITEMS_MAX];
    int house_alive, house_remain, house_win_alive;
    uint16_t house_goal;
    /* the solid map over every cell */
    unsigned char solid[SIM_MAX_CELLS_W * SIM_MAX_CELLS_H];
    /* fx pools */
    int fx_smoke, fx_popups, fx_barks, fx_sinks;
    /* the view module's clocks */
    double view_time;
    float clocks[32];
    /* the pushed draw items of one full draw pass */
    ViewStreamSnap layer[3];
} NewGameSnapshot;

/* Each snapshot carries three full view layers (~0.8 MB): keep them
 * off the stack. */
static NewGameSnapshot snap_a, snap_b;

static void capture_layer(ViewStreamSnap *into, ViewLayer layer)
{
    int count = 0;
    const ViewItem *items = view_items(layer, &count);
    into->count = count;
    if (count > 0)
        memcpy(into->items, items, sizeof(ViewItem) * (size_t)count);
    for (int i = 0; i < count; i++) into->order[i] = view_order_at(layer, i);
}

static void capture_new_game_snapshot(NewGameSnapshot *s)
{
    memset(s, 0, sizeof(*s));
    s->tick = world.tick;
    s->room_loaded_tick = world.room_loaded_tick;
    s->room_index = world.room_index;
    s->cells_w = world.cells_w;
    s->cells_h = world.cells_h;
    s->shadows_present = world.shadows_present;
    s->rng = world.rng;

    s->dog_alive = dog_alive();
    s->dog_play = dog_play();
    s->dog_cx = dog_cx();
    s->dog_cy = dog_cy();
    s->dog_dir = dog_dir();
    s->dog_length = dog_length();
    s->dog_strain = dog_strain();
    s->dog_vx = dog_visual_x();
    s->dog_vy = dog_visual_y();
    for (int i = 0; i < dog_length(); i++) s->dog_parts[i] = dog_part_cell(i);
    s->dog_detached = dog_detached_cell();

    s->box_cnt = box_count();
    for (int i = 0; i < s->box_cnt; i++) {
        s->box[i].alive = box_alive(i);
        s->box[i].cell = box_cell(i);
    }
    for (int i = 0; i < DOOR_MAX; i++) {
        s->door[i].open = door_open(i);
        s->door[i].e.alive = door_alive(i);
        s->door[i].e.cell = door_cell(i);
    }
    s->apple_cnt = apple_count();
    for (int i = 0; i < s->apple_cnt; i++) {
        s->apple[i].alive = apple_alive(i);
        s->apple[i].cell = apple_cell(i);
    }
    s->pear_cnt = pear_count();
    for (int i = 0; i < s->pear_cnt; i++) {
        s->pear[i].alive = pear_alive(i);
        s->pear[i].cell = pear_cell(i);
    }
    s->house_alive = house_alive();
    s->house_remain = house_remain();
    s->house_win_alive = house_win_alive();
    s->house_goal = house_goal_cell();

    for (int cy = 0; cy < SIM_MAX_CELLS_H; cy++)
        for (int cx = 0; cx < SIM_MAX_CELLS_W; cx++)
            s->solid[sim_cell_of(cx, cy)] =
                (unsigned char)solid_kind_at(sim_cell_of(cx, cy));

    fx_counts(&s->fx_smoke, &s->fx_popups, &s->fx_barks, &s->fx_sinks);

    /* the animation clocks, then one full read-only draw pass (world_draw
     * begins the frame and pushes every object's items) */
    s->view_time = view_time_ms();
    for (int sp = 0; sp < 32; sp++) s->clocks[sp] = view_sprite_clock(sp);
    world_draw();
    capture_layer(&s->layer[0], VIEW_SHADOW);
    capture_layer(&s->layer[1], VIEW_WORLD);
    capture_layer(&s->layer[2], VIEW_GUI);
}

static void assert_new_game_snapshots_equal(const NewGameSnapshot *a,
                                            const NewGameSnapshot *b)
{
    assert(a->tick == b->tick);
    assert(a->room_loaded_tick == b->room_loaded_tick);
    assert(a->room_index == b->room_index);
    assert(a->cells_w == b->cells_w && a->cells_h == b->cells_h);
    assert(a->shadows_present == b->shadows_present);
    assert(a->rng == b->rng); /* the gameplay stream advanced in lockstep */

    assert(a->dog_alive == b->dog_alive);
    assert(a->dog_play == b->dog_play);
    assert(a->dog_cx == b->dog_cx && a->dog_cy == b->dog_cy);
    assert(a->dog_dir == b->dog_dir && a->dog_length == b->dog_length);
    assert(a->dog_strain == b->dog_strain);
    assert(a->dog_vx == b->dog_vx && a->dog_vy == b->dog_vy);
    assert(memcmp(a->dog_parts, b->dog_parts, sizeof(a->dog_parts)) == 0);
    assert(a->dog_detached == b->dog_detached);

    assert(a->box_cnt == b->box_cnt);
    assert(memcmp(a->box, b->box, sizeof(a->box)) == 0);
    assert(memcmp(a->door, b->door, sizeof(a->door)) == 0);
    assert(a->apple_cnt == b->apple_cnt && a->pear_cnt == b->pear_cnt);
    assert(memcmp(a->apple, b->apple, sizeof(a->apple)) == 0);
    assert(memcmp(a->pear, b->pear, sizeof(a->pear)) == 0);
    assert(a->house_alive == b->house_alive);
    assert(a->house_remain == b->house_remain);
    assert(a->house_win_alive == b->house_win_alive);
    assert(a->house_goal == b->house_goal);

    assert(memcmp(a->solid, b->solid, sizeof(a->solid)) == 0);

    assert(a->fx_smoke == b->fx_smoke && a->fx_popups == b->fx_popups);
    assert(a->fx_barks == b->fx_barks && a->fx_sinks == b->fx_sinks);

    /* the view module's frame time and sprite clocks must restart with
     * the new game: after the same number of updates they match exactly
     * (they used to survive sim_init, so the second run drifted) */
    assert(a->view_time == b->view_time);
    assert(memcmp(a->clocks, b->clocks, sizeof(a->clocks)) == 0);

    /* and the full pushed draw streams replay identically */
    for (int l = 0; l < 3; l++) {
        assert(a->layer[l].count == b->layer[l].count);
        assert(memcmp(a->layer[l].items, b->layer[l].items,
                      sizeof(ViewItem) * (size_t)a->layer[l].count) == 0);
        assert(memcmp(a->layer[l].order, b->layer[l].order,
                      sizeof(int) * (size_t)a->layer[l].count) == 0);
    }
}

static void run_new_game_script(void)
{
    start_playable_in_tutorial(); /* sim_init(42u) -> title -> tutorial */
    press_dir(90);
    press_dir(90);
}

static void test_new_game_is_deterministic(void)
{
    run_new_game_script();
    capture_new_game_snapshot(&snap_a);

    run_new_game_script(); /* the identical new game + script, again */
    capture_new_game_snapshot(&snap_b);

    assert_new_game_snapshots_equal(&snap_a, &snap_b);
}

/* ---------------------------------------------------------------- */
/* The 60 Hz clock and the event delivery are owned by one schedule.  */

static SoundEvent delivered[EVENTS_MAX_SOUNDS];
static int delivered_count;
static int deliver_calls;

static void recording_deliver(void *user)
{
    (void)user;
    delivered_count = events_poll_sounds(delivered);
    deliver_calls++;
}

typedef struct ClockSnapshot {
    long tick;
    int room_index;
    int dog_alive, dog_play, dog_cx, dog_cy, dog_dir, dog_length;
    unsigned int rng;
    double view_time;
    ViewStreamSnap layer[3];
} ClockSnapshot;

/* ~0.8 MB per snapshot: keep the buffers off the stack. */
static ClockSnapshot clock_a, clock_b;

static void capture_clock_snapshot(ClockSnapshot *s)
{
    memset(s, 0, sizeof(*s));
    s->tick = world.tick;
    s->room_index = world.room_index;
    s->dog_alive = dog_alive();
    s->dog_play = dog_play();
    s->dog_cx = dog_cx();
    s->dog_cy = dog_cy();
    s->dog_dir = dog_dir();
    s->dog_length = dog_length();
    s->rng = world.rng;
    s->view_time = view_time_ms();
    world_draw();
    capture_layer(&s->layer[0], VIEW_SHADOW);
    capture_layer(&s->layer[1], VIEW_WORLD);
    capture_layer(&s->layer[2], VIEW_GUI);
}

static void assert_clock_snapshots_equal(const ClockSnapshot *a,
                                         const ClockSnapshot *b)
{
    assert(a->tick == b->tick);
    assert(a->room_index == b->room_index);
    assert(a->dog_alive == b->dog_alive && a->dog_play == b->dog_play);
    assert(a->dog_cx == b->dog_cx && a->dog_cy == b->dog_cy);
    assert(a->dog_dir == b->dog_dir && a->dog_length == b->dog_length);
    assert(a->rng == b->rng);
    assert(a->view_time == b->view_time);
    for (int l = 0; l < 3; l++) {
        assert(a->layer[l].count == b->layer[l].count);
        assert(memcmp(a->layer[l].items, b->layer[l].items,
                      sizeof(ViewItem) * (size_t)a->layer[l].count) == 0);
        assert(memcmp(a->layer[l].order, b->layer[l].order,
                      sizeof(int) * (size_t)a->layer[l].count) == 0);
    }
}

/* six spaced steps (right/left alternate), `draws` full draw passes
 * between consecutive updates */
static void run_clock_script(int draws)
{
    SimInput in;
    start_playable_in_tutorial();
    for (int i = 0; i < 6; i++) {
        memset(&in, 0, sizeof(in));
        in.pressed_right = (i % 2 == 0);
        in.pressed_left = (i % 2 == 1);
        tick_with(&in);
        tick_idle(1);
        for (int d = 0; d < draws; d++) world_draw();
    }
}

static void test_clock_and_event_delivery(void)
{
    /* (a) drawing never advances animation time: extra draw passes
     * between updates are free, so the same script ends in the same
     * state with the same pushed items whether it drew 0 or 3 extra
     * passes per update (the view clock used to advance on draw) */
    {
        double t;
        run_clock_script(0);
        t = view_time_ms();
        world_draw();
        world_draw();
        world_draw();
        assert(view_time_ms() == t);
        capture_clock_snapshot(&clock_a);
    }
    run_clock_script(3);
    capture_clock_snapshot(&clock_b);
    assert_clock_snapshots_equal(&clock_a, &clock_b);

    /* (b) 60 Hz vs 144 Hz frame schedules: two simulated seconds drive
     * exactly the same 120 updates through sim_frame and land in the
     * same state.  The right-press is sampled at the same point in the
     * update stream (when 60 updates have run) in both schedules, so it
     * is consumed by the same update and even the eased view floats
     * carry the identical convergence history. */
    {
        long tick0;
        start_playable_in_tutorial();
        tick0 = world.tick;

        for (int i = 0; i < 120; i++) { /* a 60 Hz frame schedule */
            SimInput in;
            memset(&in, 0, sizeof(in));
            in.pressed_right = (world.tick - tick0 == 60);
            sim_frame(SIM_STEP_MS, &in, NULL, NULL);
        }
        capture_clock_snapshot(&clock_a);
        assert(world.tick - tick0 == 120); /* one update per 1/60 s */

        start_playable_in_tutorial(); /* identical setup, 144 Hz frames */
        tick0 = world.tick;
        for (int i = 0; i < 289; i++) { /* a 144 Hz frame schedule */
            SimInput in;
            memset(&in, 0, sizeof(in));
            in.pressed_right = (world.tick - tick0 == 60);
            sim_frame(1000.0 / 144.0, &in, NULL, NULL);
        }
        capture_clock_snapshot(&clock_b);
        assert(world.tick - tick0 == 120); /* 289/144 s = 120 updates */
        assert_clock_snapshots_equal(&clock_a, &clock_b);
    }

    /* (c) a press sampled on a zero-update frame is held, then steps
     * exactly once when the next update runs */
    {
        int x0;
        start_playable_in_tutorial();
        x0 = dog_cx();
        {
            SimInput press;
            memset(&press, 0, sizeof(press));
            press.pressed_right = 1;
            sim_frame(3.0, &press, NULL, NULL); /* 3 ms: no update yet */
            assert(dog_cx() == x0);
        }
        sim_frame(SIM_STEP_MS, NULL, NULL, NULL); /* one update runs */
        assert(dog_cx() == x0 + 1);
        for (int i = 0; i < 10; i++)
            sim_frame(SIM_STEP_MS, NULL, NULL, NULL);
        assert(dog_cx() == x0 + 1); /* and it stepped exactly once */

        /* a massive stall runs the catch-up cap and drops the backlog;
         * the next frame resumes with exactly one more update */
        start_playable_in_tutorial();
        x0 = dog_cx();
        {
            long t0 = world.tick;
            sim_frame(2000.0, NULL, NULL, NULL); /* 2 s of stall */
            assert(world.tick - t0 == SIM_MAX_CATCHUP);
            sim_frame(SIM_STEP_MS, NULL, NULL, NULL);
            assert(world.tick - t0 == SIM_MAX_CATCHUP + 1);
        }
        assert(dog_cx() == x0); /* no held input replayed after the drop */
    }

    /* (d) an update's events are delivered after that update and are
     * gone before the next one: a space press barks once, the sound
     * hook sees exactly that bark, and the next update delivers
     * nothing (no cross-update leakage) */
    {
        int smoke, popups, barks, sinks;
        SimInput press;
        start_playable_in_tutorial();
        deliver_calls = 0;
        delivered_count = 0;
        memset(&press, 0, sizeof(press));
        press.pressed_space = 1;
        sim_frame(SIM_STEP_MS, &press, recording_deliver, NULL);
        assert(deliver_calls == 1);
        assert(delivered_count == 1 && delivered[0].sound == SND_BARK);
        assert(events_poll_sounds(NULL) == 0); /* the queue was drained */
        fx_counts(&smoke, &popups, &barks, &sinks);
        assert(barks == 1); /* the fx event fed that update's view pass */
        sim_frame(SIM_STEP_MS, NULL, recording_deliver, NULL);
        assert(deliver_calls == 2 && delivered_count == 0);
    }
}

/* ---------------------------------------------------------------- */
/* The draw stream describes the final composition                    */
/* ---------------------------------------------------------------- */

/* font-1 UI text items pushed into a layer (the title prompt, the
 * LEVEL label; not the in-world digits font) */
static int count_ui_texts(ViewLayer layer)
{
    int count;
    const ViewItem *items = view_items(layer, &count);
    int found = 0;
    for (int i = 0; i < count; i++)
        if (items[i].kind == VIEW_ITEM_TEXT && items[i].font_id == 1)
            found++;
    return found;
}

static int count_shadow_composites(void)
{
    int count;
    const ViewItem *items = view_items(VIEW_WORLD, &count);
    int found = 0;
    for (int i = 0; i < count; i++)
        if (items[i].kind == VIEW_ITEM_SHADOW_COMPOSITE) found++;
    return found;
}

/* The replay draws text in its sorted composition position (world-layer
 * text under the GUI layer, GUI-layer text on top of its own layer), so
 * the stream must carry what the final frame needs: the title prompt
 * stays pushed during wipes, the GUI depth order puts the wipe under
 * the depth-0 text, and the shadow composite item carries the dog/shadow
 * decision instead of the replay querying gameplay state. */
static void test_draw_composition_order(void)
{
    SimInput input;
    memset(&input, 0, sizeof(input));

    sim_init(42u);

    /* the title prompt is world-layer text: two font-1 items, and the
     * shadow composite is emitted (title room has shadows, dog alive) */
    world_draw();
    assert(count_ui_texts(VIEW_WORLD) == 2);
    assert(count_ui_texts(VIEW_GUI) == 0);
    assert(count_shadow_composites() == 1);

    input.pressed_any = 1;
    tick_with(&input);
    assert(transition_state()->phase == TRANSITION_OPENING);

    /* during the wipe the prompt stays in the stream (the old stream
     * hid it because the replay deferred all text above the GUI) and
     * the GUI wipe items that now cover it are pushed */
    world_draw();
    assert(count_ui_texts(VIEW_WORLD) == 2);
    {
        int count;
        const ViewItem *items = view_items(VIEW_GUI, &count);
        int wipes = 0;
        for (int i = 0; i < count; i++)
            if (items[i].kind == VIEW_ITEM_SPRITE &&
                items[i].sprite == LONGO_SPR_TRANSITION)
                wipes++;
        assert(wipes == 8); /* 4 rows x shadow + fill */
    }

    /* ride the wipe into the tutorial room; its dialogue is active */
    tick_idle(200);
    assert(world.room_index == SIM_ROOM_TUTORIAL);
    assert(dialogue_active());

    /* request a retry so the wipe and the dialogue share VIEW_GUI, then
     * check the sorted draw order: higher depth sorts earlier (drawn
     * further back), so the depth-2 wipe sprites and the depth-1 wipe
     * rect + dialogue panel draw before the depth-0 text — the text's
     * sorted position is its true position, nothing defers it */
    input.pressed_r = 1;
    tick_with(&input);
    assert(transition_state()->phase == TRANSITION_OPENING);

    world_draw();
    /* the replay sorts at frame start; emulate it before reading the
     * sorted draw order */
    view_sort();
    {
        int count;
        const ViewItem *items = view_items(VIEW_GUI, &count);
        int last_cover_pos = -1, first_text_pos = count;
        for (int pos = 0; pos < count; pos++) {
            const ViewItem *it = &items[view_order_at(VIEW_GUI, pos)];
            if (it->depth > 0) {
                last_cover_pos = pos;
            } else {
                /* everything at depth 0 in the GUI layer is text */
                assert(it->kind == VIEW_ITEM_TEXT ||
                       it->kind == VIEW_ITEM_TEXT_WRAPPED);
                if (pos < first_text_pos) first_text_pos = pos;
            }
        }
        assert(last_cover_pos >= 0);
        assert(first_text_pos < count);
        assert(first_text_pos > last_cover_pos);
    }

    /* the composite item is the only shadow decision the replay gets:
     * emitted while the dog lives, gone when it does not */
    dog_set_alive(false);
    world_draw();
    assert(count_shadow_composites() == 0);
    dog_set_alive(true);
    world_draw();
    assert(count_shadow_composites() == 1);
}

/* ---------------------------------------------------------------- */
/* One catalog, one loading policy: the room tables in level_data.c    */
/* are the only definition, every shipped room validates clean, and    */
/* play order / tile maps / dialogue all key off that catalog.         */

static void test_room_catalog_loads_clean(void)
{
    /* every authored room passes the loader's validation: dimensions
     * fit the sim grid exactly and no entity pool overflows (offscreen
     * placements are whitelisted decoration, only counted).  sim_init
     * below also re-checks the whole catalog, so malformed data would
     * abort with the room's name before this loop even runs. */
    sim_init(42u);
    for (int i = 0; i < longo_room_count(); i++) {
        LongoRoomValidation report;
        const LongoRoom *room = longo_room(i);
        assert(room != NULL);
        assert(longo_room_validate(i, &report));
        assert(report.violations == 0);
        /* the tile map is keyed off the same catalog identity */
        assert(room_tiles_for(room) != NULL);
        assert(room_tiles_for(room)->room_id == room->id);
        assert(longo_room_tile_maps[i].room_id == i); /* indexed by id */
    }
    assert(longo_room(-1) == NULL);
    assert(longo_room(longo_room_count()) == NULL);

    /* the catalog owns play order: the title is room 0, the shipped
     * prefix matches the sim's named play rooms, names and ids are
     * unique, and the two authored-but-unshipped rooms (editor,
     * levelbase) trail out of play */
    assert(longo_room_count() == 11);
    assert(longo_room_play_count() == SIM_ROOM_CREDITS + 1);
    assert(strcmp(longo_room(SIM_ROOM_TITLE)->name, "rm_title_screen") == 0);
    assert(longo_room(SIM_ROOM_TITLE)->id == LONGO_ROOM_ID_TITLE_SCREEN);
    for (int i = 0; i < longo_room_count(); i++) {
        for (int j = i + 1; j < longo_room_count(); j++) {
            assert(strcmp(longo_room(i)->name, longo_room(j)->name) != 0);
            assert(longo_room(i)->id != longo_room(j)->id);
        }
        assert(longo_room(i)->width % SIM_CELL == 0);
        assert(longo_room(i)->height % SIM_CELL == 0);
    }

    /* transcription guard: the moved tables must match the original
     * header-defined data for spot-checked rooms (row counts, first and
     * last rows, anchors and the fractional wall scales) */
    {
        const LongoRoom *title = longo_room(SIM_ROOM_TITLE);
        const LongoRoom *tutorial = longo_room(SIM_ROOM_TUTORIAL);
        const LongoRoom *level1 = longo_room(SIM_ROOM_LEVEL1);
        assert(title->width == 304 && title->height == 208);
        assert(title->object_count == 34);
        assert(title->objects[0].object == LONGO_OBJ_FLOWER);
        assert(title->objects[0].x == 0 && title->objects[0].y == 224);
        assert(title->objects[0].depth == -300);
        assert(title->objects[5].object == LONGO_OBJ_WIN);
        assert(title->objects[5].x == 136 && title->objects[5].xscale == 2);
        assert(title->objects[33].object == LONGO_OBJ_FLOWER);
        assert(title->objects[33].x == 96 && title->objects[33].y == 40);
        assert(tutorial->object_count == 25);
        assert(tutorial->objects[6].object == LONGO_OBJ_BLOCK);
        assert(tutorial->objects[6].x == 0 &&
               tutorial->objects[6].xscale == 19);
        assert(tutorial->objects[8].object == LONGO_OBJ_BLOCK);
        assert(tutorial->objects[8].x == 288 &&
               tutorial->objects[8].yscale == 12.5f);
        assert(tutorial->objects[14].object == LONGO_OBJ_TUTORIAL);
        assert(tutorial->objects[14].x == 408 &&
               tutorial->objects[14].xscale == 0.6666667f);
        assert(level1->object_count == 56);
        assert(level1->objects[0].object == LONGO_OBJ_FLOWER &&
               level1->objects[0].y == 224);
        assert(level1->objects[1].object == LONGO_OBJ_HOUSESPAWNER &&
               level1->objects[1].x == 64 && level1->objects[1].y == 48);
        assert(level1->objects[34].object == LONGO_OBJ_BLOCK);
        assert(level1->objects[34].x == -16 &&
               level1->objects[34].xscale == 21);
        assert(level1->objects[55].object == LONGO_OBJ_APPLE);
        assert(level1->objects[55].x == 344 && level1->objects[55].y == 120);
    }

    /* offscreen functional placements never load: the loader skips any
     * cell-based object whose computed cell lands outside the room grid
     * (the same placements the validator counts as authored
     * decoration), so an oversized pixel coordinate cannot wrap through
     * the sim-grid stride onto a playable cell.  rm_level6 ships an
     * apple at (384,160) whose cell (24,10) used to pack onto playable
     * cell (0,11). */
    for (int i = 0; i < longo_room_play_count(); i++) {
        const LongoRoom *room = longo_room(i);
        int expect_apples = 0, expect_pears = 0;
        int alive_apples = 0, alive_pears = 0;
        for (int j = 0; j < room->object_count; j++) {
            const LongoRoomObject *p = &room->objects[j];
            int cx = (int)floorf(p->x / SIM_CELL);
            int cy = (int)floorf(p->y / SIM_CELL);
            if (cx < 0 || cy < 0 || cx >= room->width / SIM_CELL ||
                cy >= room->height / SIM_CELL)
                continue;
            if (p->object == LONGO_OBJ_APPLE) expect_apples++;
            if (p->object == LONGO_OBJ_PEAR) expect_pears++;
        }
        sim_room_goto(world_ptr(), i);
        tick_idle(2);
        for (int j = 0; j < apple_count(); j++) {
            if (!apple_alive(j)) continue;
            alive_apples++;
            assert(sim_cell_x(apple_cell(j)) < world.cells_w);
            assert(sim_cell_y(apple_cell(j)) < world.cells_h);
        }
        for (int j = 0; j < pear_count(); j++) {
            if (!pear_alive(j)) continue;
            alive_pears++;
            assert(sim_cell_x(pear_cell(j)) < world.cells_w);
            assert(sim_cell_y(pear_cell(j)) < world.cells_h);
        }
        for (int j = 0; j < box_count(); j++) {
            if (!box_alive(j)) continue;
            assert(sim_cell_x(box_cell(j)) < world.cells_w);
            assert(sim_cell_y(box_cell(j)) < world.cells_h);
        }
        assert(alive_apples == expect_apples);
        assert(alive_pears == expect_pears);
    }
    sim_room_goto(world_ptr(), SIM_ROOM_LEVEL6);
    tick_idle(2);
    assert(apple_index_at(sim_cell_of(0, 11)) == -1);
    assert(pear_index_at(sim_cell_of(0, 11)) == -1);

    /* play progression follows catalog order and stops after the last
     * shipped room (the editor and levelbase rooms stay unreachable) */
    for (int i = 0; i + 1 < longo_room_play_count(); i++) {
        sim_room_goto(world_ptr(), i);
        tick_idle(2);
        assert(world.room_index == i);
        assert(world.room == longo_room(i));
        sim_room_goto_next(world_ptr());
        tick_idle(2);
        assert(world.room_index == i + 1);
    }
    sim_room_goto(world_ptr(), longo_room_play_count() - 1);
    tick_idle(2);
    sim_room_goto_next(world_ptr());
    tick_idle(2);
    assert(world.room_index == longo_room_play_count() - 1);
    sim_room_goto(world_ptr(), longo_room_play_count());
    tick_idle(2);
    assert(world.room_index == longo_room_play_count() - 1);

    /* dialogue selection keys off the same catalog play indices (the
     * switch in objects/dialogue.c is authored content) */
    for (int i = 0; i < longo_room_play_count(); i++) {
        int wants_dialogue =
            (i == SIM_ROOM_TUTORIAL || i == SIM_ROOM_LEVEL6 ||
             i == SIM_ROOM_LEVEL4 || i == SIM_ROOM_CREDITS);
        sim_room_goto(world_ptr(), i);
        tick_idle(2);
        assert(dialogue_active() == wants_dialogue);
    }
}

/* Registered in CTest with WILL_FAIL: it must exit non-zero in every
 * build configuration.  The side effect inside the assert proves check
 * expressions really evaluate; if -DNDEBUG ever strips them again, the
 * probe returns 0 and ctest's WILL_FAIL flags the hollow suite.  It
 * exits cleanly instead of aborting so WILL_FAIL sees a plain failure. */
static void scenario_fail_probe(void)
{
    volatile int evaluated = 0;
    assert(++evaluated);
    if (!evaluated) return; /* checks compiled out: probe "passes" */
    fprintf(stderr, "fail_probe: checks are active, failing as designed\n");
    exit(1);
}

typedef struct Scenario {
    const char *name;
    void (*fn)(void);
} Scenario;

static const Scenario scenarios[] = {
    { "title_flow_room_order", test_title_flow_and_room_order },
    { "tutorial_dialogue_gates_play", test_tutorial_dialogue_gates_play },
    { "movement_and_chain", test_movement_and_chain },
    { "room_load_rules", test_room_load_rules },
    { "apple_pear_length", test_apple_and_pear_length },
    { "walls_and_push_rules", test_walls_and_push_rules },
    { "buttons_door_win_retry", test_buttons_door_win_retry },
    { "retry_reloads_room", test_retry_reloads_room },
    { "room_flow_requests", test_room_flow_requests },
    { "fx_pools_reuse_dead_slots", test_fx_pools_reuse_dead_slots },
    { "undo", test_undo },
    { "door_open_window_solidity", test_door_open_window_solidity },
    { "occupancy_matches_entities", test_occupancy_matches_entities },
    { "new_game_is_deterministic", test_new_game_is_deterministic },
    { "clock_and_event_delivery", test_clock_and_event_delivery },
    { "draw_composition_order", test_draw_composition_order },
    { "room_catalog_loads_clean", test_room_catalog_loads_clean },
};

static void run_scenario(const Scenario *s)
{
    printf("[scenario] %s\n", s->name);
    fflush(stdout);
    s->fn();
}

static int run_all(void)
{
    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++)
        run_scenario(&scenarios[i]);
    printf("longo_game_smoke: all tests passed\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--list") == 0) {
        for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++)
            printf("%s\n", scenarios[i].name);
        printf("fail_probe\n");
        return 0;
    }
    if (argc >= 2) {
        for (int a = 1; a < argc; a++) {
            if (strcmp(argv[a], "fail_probe") == 0) {
                run_scenario(&(Scenario){ "fail_probe", scenario_fail_probe });
                continue;
            }
            int found = 0;
            for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]);
                 i++) {
                if (strcmp(argv[a], scenarios[i].name) == 0) {
                    run_scenario(&scenarios[i]);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "unknown scenario: %s\n", argv[a]);
                return 2;
            }
        }
        printf("longo_game_smoke: selected scenarios passed\n");
        return 0;
    }
    return run_all();
}
