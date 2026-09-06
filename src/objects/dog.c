/*
 * Dog rules: fully tile-based.
 *
 * The head occupies a cell; a step snaps it into the adjacent cell
 * instantly and the body chain shifts one cell along (classic snake).
 * Movement rules, in cell terms:
 *   - one instant 16px logical step per accepted press, facing update
 *   - the step is allowed by solid_probe() and moved by box_push()
 *   - the tail exception: the last chain cell does not block, because it
 *     vacates in the same tick
 *   - pickups trigger on the head's new cell
 */
#include "dog.h"

#include <math.h>
#include <string.h>

#include "../core/events.h"
#include "../core/sim_math.h"
#include "../core/undo.h"
#include "../core/view.h"
#include "../core/solid.h"
#include "box.h"
#include "house.h"
#include "items.h"
#include "title.h"
#include "transition.h"

/* key_cooldown is 2 after each step: it gates the next press for two
 * ticks, so even mashing moves at most one cell every 2 ticks */
#define DOG_KEY_COOLDOWN 2

static Dog dog;

static uint16_t head_cell(void) { return sim_cell_of(dog.cx, dog.cy); }

static int sign(int v) { return (v > 0) - (v < 0); }

/* Placement must initialise the eased view position (a fresh object
 * never eases in from the origin). */
static void view_snap(void);
static void view_part_snap(int index, uint16_t cell);

/* ------------------------------------------------------------------ */
/* Placement                                                           */
/* ------------------------------------------------------------------ */

static void place_on_solid_map(void)
{
    solid_place(head_cell(), SOLID_HEAD, 0);
    for (int i = 0; i < dog.length; i++)
        solid_place(dog.chain[i], SOLID_BODY, i);
}

void dog_reset(void)
{
    memset(&dog, 0, sizeof(dog));
    dog.bark_timer = -1;
}

void dog_capture(Dog *out) { *out = dog; }

void dog_restore(const Dog *snap)
{
    dog = *snap;
    view_snap();
}

void dog_place(float x, float y)
{
    dog.alive = true;
    dog.cx = (int)floorf((x - 8.0f) / SIM_CELL);
    dog.cy = (int)floorf((y - 8.0f) / SIM_CELL);
    dog.dir = 180; /* facing up, chain trailing below */
    dog.play = true;
    dog.length = 5;
    dog.strain = false;
    dog.key_cooldown = 0;
    dog.bark_timer = -1;
    for (int i = 0; i < dog.length; i++) {
        dog.chain[i] = sim_cell_of(dog.cx, dog.cy + 1 + i);
        dog.pflag[i] = DOG_PART_BUTT;
        if (i == 0) dog.pflag[i] |= DOG_PART_FIRST | DOG_PART_LEGS;
        if (i == dog.length - 1) dog.pflag[i] = DOG_PART_LEGS;
    }
    dog.detached_cell = dog.chain[dog.length - 1];
    place_on_solid_map();
    view_snap();
}

/* The title room rearranges the dog into an S-curve and arms its idle
 * bark. */
void dog_title_arrangement(void)
{
    /* head first, then the five parts of the S-curve (pixel coords,
     * snapped to cells) */
    static const int cells[6][2] = {
        { 10, 10 }, { 10, 9 }, { 9, 9 }, { 8, 9 }, { 8, 10 }, { 9, 10 }
    };
    if (!dog.alive) return;
    dog.cx = cells[0][0];
    dog.cy = cells[0][1];
    dog.dir = 0;
    dog.bark_timer = 10;
    for (int i = 0; i < dog.length && i + 1 < 6; i++)
        dog.chain[i] = sim_cell_of(cells[i + 1][0], cells[i + 1][1]);
    place_on_solid_map();
    view_snap();
}

/* ------------------------------------------------------------------ */
/* Accessors                                                           */
/* ------------------------------------------------------------------ */

bool dog_alive(void) { return dog.alive; }
bool dog_play(void) { return dog.play; }
void dog_set_play(bool play) { dog.play = play; }
int dog_cx(void) { return dog.cx; }
int dog_cy(void) { return dog.cy; }
int dog_dir(void) { return dog.dir; }
int dog_length(void) { return dog.length; }
uint16_t dog_part_cell(int part) { return dog.chain[part]; }
int dog_part_flags(int part) { return dog.pflag[part]; }
bool dog_part_is_solid(int part)
{
    return part >= 0 && part < dog.length && (dog.pflag[part] & DOG_PART_BUTT);
}
bool dog_strain(void) { return dog.strain; }
uint16_t dog_detached_cell(void) { return (uint16_t)dog.detached_cell; }

void dog_teleport(int cx, int cy)
{
    /* clear old cells, re-place the head with the chain trailing below
     * like a fresh spawn (test hook) */
    solid_clear(head_cell());
    for (int i = 0; i < dog.length; i++) solid_clear(dog.chain[i]);
    dog.cx = cx;
    dog.cy = cy;
    dog.key_cooldown = 0;
    for (int i = 0; i < dog.length; i++)
        dog.chain[i] = sim_cell_of(cx, cy + 1 + i);
    place_on_solid_map();
    view_snap();
}

void dog_set_alive(bool alive)
{
    if (dog.alive == alive) return;
    if (dog.alive) {
        solid_clear(head_cell());
        for (int i = 0; i < dog.length; i++) solid_clear(dog.chain[i]);
        dog.alive = false;
    } else {
        dog.alive = true;
        place_on_solid_map();
    }
}

void dog_set_length(int length)
{
    /* test hook: grow/shrink the chain in place around the current tail */
    if (length < 0 || length > DOG_MAX_CHAIN) return;
    while (dog.length > length) {
        solid_clear(dog.chain[dog.length - 1]);
        dog.length--;
        if (dog.length > 0) dog.pflag[dog.length - 1] = DOG_PART_LEGS;
    }
    while (dog.length < length) {
        dog.chain[dog.length] = dog.chain[dog.length - 1];
        dog.pflag[dog.length] = DOG_PART_LEGS;
        view_part_snap(dog.length, dog.chain[dog.length]);
        dog.length++;
    }
    place_on_solid_map();
}

/* ------------------------------------------------------------------ */
/* Movement                                                            */
/* ------------------------------------------------------------------ */

static void shift_chain(uint16_t old_head)
{
    /* The cell the tail vacates is where a grown segment reappears.  A
     * box may have just been pushed onto it (the tail is not solid), so
     * only clear it when it still holds a body part. */
    uint16_t tail = dog.chain[dog.length - 1];
    if (solid_kind_at(tail) == SOLID_BODY) solid_clear(tail);
    dog.detached_cell = tail;
    for (int i = dog.length - 1; i > 0; i--) dog.chain[i] = dog.chain[i - 1];
    dog.chain[0] = old_head;
    /* re-stamp every part so the map's indices stay in step with the
     * chain (the tail exception probes dog_part_is_solid by index) */
    place_on_solid_map();
}

static void grow_chain(uint16_t at_cell)
{
    if (dog.length >= DOG_MAX_CHAIN) return;
    /* the former tail becomes plain body; the new tail gets legs */
    dog.pflag[dog.length - 1] = DOG_PART_BUTT;
    dog.chain[dog.length] = at_cell;
    dog.pflag[dog.length] = DOG_PART_LEGS;
    /* the new part starts in the cell that just emptied, and its eased
     * view position snaps there */
    view_part_snap(dog.length, at_cell);
    dog.length++;
    place_on_solid_map();
}

static void shrink_chain(void)
{
    solid_clear(dog.chain[dog.length - 1]);
    dog.length--;
    dog.pflag[dog.length - 1] = DOG_PART_LEGS;
    place_on_solid_map();
}

static void emit_bark(void)
{
    float hx = (float)(sim_cell_x(head_cell()) * SIM_CELL + 8);
    float hy = (float)(sim_cell_y(head_cell()) * SIM_CELL + 8);
    float fx = hx, fy = hy;
    switch (dog.dir) {
    case 0: fy += 12.0f; break;
    case 90: fx += 12.0f; break;
    case 180: fy -= 12.0f; break;
    default: fx -= 12.0f; break;
    }
    events_fx(FX_BARK, fx, fy, 0, 0, (float)dog.dir + 180.0f, 0, 0);
    events_sound(SND_BARK, 0);
}

/* Pickups on the head's new cell (apple, pear, win zone). */
static void resolve_pickups(void)
{
    uint16_t head = head_cell();
    float hx = (float)(sim_cell_x(head) * SIM_CELL + 8);
    float hy = (float)(sim_cell_y(head) * SIM_CELL + 8);

    int apple = apple_index_at(head);
    if (apple >= 0) {
        apple_consume(apple);
        grow_chain(dog.detached_cell);
        events_fx(FX_ONE, hx, hy - 8.0f, 0, 0, 0, 0, 0);
        events_fx(FX_SMOKE_BURST, hx, hy, 0, 0, 0, 7, 0);
        events_sound(SND_POOF, 0);
    }

    int pear = pear_index_at(head);
    if (pear >= 0) {
        pear_consume(pear);
        if (dog.length > 2) {
            uint16_t tail = dog.chain[dog.length - 1];
            shrink_chain();
            events_fx(FX_SMOKE_BURST,
                      (float)(sim_cell_x(tail) * SIM_CELL + 8),
                      (float)(sim_cell_y(tail) * SIM_CELL + 8), 0, 0, 0, 7, 0);
            events_fx(FX_ONE, hx, hy - 8.0f, 0, 0, 0, 0, 1);
        }
        /* at the minimum three-piece shape (head, middle, butt) a
         * pear is eaten with no further effect */
        events_sound(SND_POOF, 0);
    }

    if (dog.alive && house_try_win(head)) {
        transition_count_win();
        transition_request_next();
    }
}

/* Attempt one cell step; returns true on success.  Every board change
 * in the game flows through here, which makes it the one undo point:
 * the capture lands before the first mutation and is only kept when the
 * step actually moves (a strained step pushes no phantom history). */
static bool try_step(int dir)
{
    undo_begin_step();
    uint16_t target = cell_neighbour(head_cell(), dir);
    int tcx = sim_cell_x(target);
    int tcy = sim_cell_y(target);

    if (!cell_in_bounds(tcx, tcy)) return false;

    switch (solid_probe(target)) {
    case SOLID_PROBE_SOLID:
        dog.strain = true;
        return false;
    case SOLID_PROBE_BOX:
        if (!box_push(solid_index_at(target), target, dir)) {
            dog.strain = true;
            return false;
        }
        break;
    default:
        break;
    }

    uint16_t old_head = head_cell();
    dog.cx = tcx;
    dog.cy = tcy;
    dog.dir = dir;
    dog.strain = false;
    solid_place(target, SOLID_HEAD, 0);
    shift_chain(old_head);
    undo_commit_step();
    return true;
}

void dog_tick(const SimInput *input)
{
    if (!dog.alive) return;

    /* R retries unless a wipe is closing */
    if (input->pressed_r && !transition_closing()) transition_request_retry();

    /* the title screen parks the dog */
    if (title_present()) dog.play = false;

    if (dog.play && input->pressed_space) emit_bark();

    if (dog.key_cooldown > 0) dog.key_cooldown--;

    /* xmove/ymove are the pressed edges; the move only fires while
     * key_cooldown is clear.  Horizontal wins diagonal input. */
    int xm = sign((input->pressed_right ? 1 : 0) - (input->pressed_left ? 1 : 0));
    int ym = sign((input->pressed_down ? 1 : 0) - (input->pressed_up ? 1 : 0));

    if (dog.play && dog.key_cooldown == 0 && (xm != 0 || ym != 0)) {
        /* the press opposite to the facing is the dead input — the head
         * would step into its own neck — so it backs the last move out
         * instead of straining */
        int dir = xm != 0 ? (xm > 0 ? 90 : 270) : (ym > 0 ? 0 : 180);
        bool acted = dir == (dog.dir + 180) % 360 ? undo_pop() : try_step(dir);
        if (acted) dog.key_cooldown = DOG_KEY_COOLDOWN;
    }

    /* idle bark (armed only by the title room) */
    if (dog.bark_timer > 0) {
        if (--dog.bark_timer == 0) {
            if (world_random(1.0f) < 0.2f) emit_bark();
            dog.bark_timer = 25;
        }
    }

    resolve_pickups();
}

/* ------------------------------------------------------------------ */
/* View: eased positions, legs wiggle, vector body and head sprite     */
/* ------------------------------------------------------------------ */

static float v_dog_x, v_dog_y;
static float v_part_x[DOG_MAX_CHAIN], v_part_y[DOG_MAX_CHAIN];
static uint16_t v_part_cell[DOG_MAX_CHAIN];
static int v_wiggle[DOG_MAX_CHAIN];

static void view_part_snap(int index, uint16_t cell)
{
    if (index < 0 || index >= DOG_MAX_CHAIN) return;
    v_part_x[index] = (float)(sim_cell_x(cell) * SIM_CELL + 8);
    v_part_y[index] = (float)(sim_cell_y(cell) * SIM_CELL + 8);
    v_part_cell[index] = cell;
    v_wiggle[index] = 0;
}

static void view_snap(void)
{
    v_dog_x = (float)(dog.cx * SIM_CELL + 8);
    v_dog_y = (float)(dog.cy * SIM_CELL + 8);
    for (int i = 0; i < dog.length; i++)
        view_part_snap(i, dog.chain[i]);
}

void dog_view_tick(void)
{
    /* eased view position; targets are sprite centres (cell top-left + 8) */
    float tx = (float)(dog.cx * SIM_CELL + 8);
    float ty = (float)(dog.cy * SIM_CELL + 8);
    if (dog.alive) {
        v_dog_x = f_lerp(v_dog_x, tx, 0.2f);
        v_dog_y = f_lerp(v_dog_y, ty, 0.2f);
    }
    for (int i = 0; i < dog.length; i++) {
        uint16_t cell = dog.chain[i];
        float ptx = (float)(sim_cell_x(cell) * SIM_CELL + 8);
        float pty = (float)(sim_cell_y(cell) * SIM_CELL + 8);
        if (v_wiggle[i] > 0) v_wiggle[i]--;
        if (v_part_cell[i] != cell) {
            v_part_cell[i] = cell;
            v_wiggle[i] = 15; /* half-way through the 30-tick leg cycle */
        }
        v_part_x[i] = f_lerp(v_part_x[i], ptx, 0.2f);
        v_part_y[i] = f_lerp(v_part_y[i], pty, 0.2f);
    }
}

float dog_visual_x(void) { return v_dog_x; }
float dog_visual_y(void) { return v_dog_y; }

static float legs_angle(int part)
{
    return v_wiggle[part] > 0 ? 30.0f : 0.0f;
}

/* Body chain first, then the head sprite; the shadow pass repeats the
 * same shapes in black, flipped below the origin like every shadow. */
static void draw_body(bool shadow)
{
    const ViewColor body = view_rgb(153, 108, 53);
    const ViewColor outline = view_rgb(107, 61, 49);
    const ViewColor black = view_rgb(0, 0, 0);
    int depth = VIEW_DEPTH_DOG;
    int face = (int)view_sprite_clock(LONGO_SPR_FLOWER) % 4;
    LongoSprite head_sprite;
    switch (dog.dir) {
    case 0: head_sprite = LONGO_SPR_DOGDOWN; break;
    case 90: head_sprite = LONGO_SPR_DOGRIGHT; break;
    case 180: head_sprite = LONGO_SPR_DOGUP; break;
    default: head_sprite = LONGO_SPR_DOGLEFT; break;
    }

    /* pass 1: legs, outline body, outline head circle */
    for (int i = 0; i < dog.length; i++) {
        float px = v_part_x[i];
        float py = v_part_y[i];
        float fx = i == 0 ? v_dog_x : v_part_x[i - 1];
        float fy = i == 0 ? v_dog_y : v_part_y[i - 1];
        bool is_first = (dog.pflag[i] & DOG_PART_FIRST) != 0;
        bool legs = (dog.pflag[i] & DOG_PART_LEGS) != 0;
        float amp = legs_angle(i);
        float legs_wave = longo_wave(-amp, amp, 0.2f, 0, view_time_ms());
        if (legs) {
            float dir = (point_direction(px, py, fx, fy) - 90.0f) +
                        (180.0f * is_first);
            float leglength = 6.0f + (is_first ? 3.0f : 0.0f);
            for (int leg = 0; leg < 2; leg++) {
                float angle = (leg == 0 ? -45.0f : 225.0f) + legs_wave + dir;
                if (shadow) {
                    view_line(depth, px, py + 5.0f,
                              px + len_dir_x(leglength, angle),
                              py + 5.0f + len_dir_y(leglength, angle), 2.0f,
                              black, black);
                    view_circle(depth,
                                px + len_dir_x(leglength, angle),
                                py + 5.0f + len_dir_y(leglength, angle), 3.0f,
                                black, black);
                } else {
                    view_line(depth, px, py,
                              px + len_dir_x(leglength, angle),
                              py + len_dir_y(leglength, angle), 2.0f, outline,
                              body);
                    view_circle(depth,
                                px + len_dir_x(leglength, angle),
                                py + len_dir_y(leglength, angle), 3.0f, body,
                                outline);
                }
            }
        }
        if (shadow) {
            view_circle(depth, px - 1.0f, py + 4.0f, 5.0f, black, black);
            view_line(depth, px - 1.0f, py + 4.0f, fx - 1.0f,
                      fy + 4.0f, 10.0f, black, black);
        } else {
            view_circle(depth, px - 1.0f, py - 1.0f, 5.0f, outline, outline);
            view_line(depth, px - 1.0f, py - 1.0f, fx - 1.0f,
                      fy - 1.0f, 10.0f, outline, outline);
            if (is_first)
                view_circle(depth, v_dog_x, v_dog_y - 1.0f,
                            5.0f, outline, outline);
        }
    }

    /* pass 2: fill body and tail sprite */
    if (!shadow) {
        for (int i = 0; i < dog.length; i++) {
            float px = v_part_x[i];
            float py = v_part_y[i];
            float fx = i == 0 ? v_dog_x : v_part_x[i - 1];
            float fy = i == 0 ? v_dog_y : v_part_y[i - 1];
            bool legs = (dog.pflag[i] & DOG_PART_LEGS) != 0;
            view_circle(depth, px - 1.0f, py - 1.0f, 4.0f, body, body);
            view_line(depth, px - 1.0f, py - 1.0f, fx - 1.0f,
                      fy - 1.0f, 8.0f, body, body);
            if (legs && !(dog.pflag[i] & DOG_PART_FIRST)) {
                int tail_frame = (int)view_sprite_clock(LONGO_SPR_FLOWER) % 4;
                view_sprite(depth, LONGO_SPR_DOGTAIL,
                            tail_frame, px - 1.0f, py - 3.0f, 1.0f, 1.0f,
                            0.0f, view_rgb(255, 255, 255), 1.0f);
            }
        }
        view_sprite(depth, head_sprite, face, v_dog_x, v_dog_y, 1.0f,
                    1.0f, 0.0f, view_rgb(255, 255, 255), 1.0f);
    } else {
        view_sprite(depth, head_sprite, face, v_dog_x, v_dog_y + 5.0f,
                    1.0f, 1.0f, 0.0f, black, 1.0f);
    }
}

void dog_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    draw_body(shadow);
}
