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
    for (int room = 0; room < SIM_ROOM_COUNT; room++) {
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
