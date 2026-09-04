/*
 * Simulation core: world state, room loading (level_data instances ->
 * cells + entities), and the tick orchestrator.  Movement rules live in
 * sim_dog.c.  Every rule here is a port of the recovered GML behaviour,
 * translated from bbox probes to cell lookups (see docs/architecture.md).
 */
#include "world.h"

#include "../objects/box.h"
#include "../objects/house.h"
#include "../objects/items.h"
#include "../objects/hole.h"
#include "events.h"
#include "solid.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

int sim_cell_x(uint16_t cell) { return cell % SIM_MAX_CELLS_W; }
int sim_cell_y(uint16_t cell) { return cell / SIM_MAX_CELLS_W; }
uint16_t sim_cell_of(int cx, int cy) { return SIM_CELL_INDEX(cx, cy); }

static int in_bounds(const SimWorld *w, int cx, int cy)
{
    return cx >= 0 && cy >= 0 && cx < w->cells_w && cy < w->cells_h;
}

static SimWorld game_world;

SimWorld *world_ptr(void) { return &game_world; }

bool cell_in_bounds(int cx, int cy)
{
    SimWorld *w = &game_world;
    return cx >= 0 && cy >= 0 && cx < w->cells_w && cy < w->cells_h;
}

uint16_t cell_neighbour(uint16_t cell, int dir)
{
    int cx = sim_cell_x(cell);
    int cy = sim_cell_y(cell);
    switch (dir) {
    case 0: return sim_cell_of(cx, cy + 1);   /* down */
    case 90: return sim_cell_of(cx + 1, cy);  /* right */
    case 180: return sim_cell_of(cx, cy - 1); /* up */
    default: return sim_cell_of(cx - 1, cy);  /* left */
    }
}

static int cell_solid(const SimWorld *w, uint16_t cell)
{
    return w->solid[cell] != 0;
}

float sim_random(SimWorld *w, float max)
{
    unsigned int x = w->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    w->rng = x ? x : 0x9e3779b9u;
    return (float)(x & 0xFFFFFF) / (float)0x1000000 * max;
}

float sim_random_range(SimWorld *w, float lo, float hi)
{
    return lo + sim_random(w, hi - lo);
}

static float f_lerp(float a, float b, float t) { return a + (b - a) * t; }

/* ------------------------------------------------------------------ */
/* Entity lookups (linear scans; the counts are tiny)                  */
/* ------------------------------------------------------------------ */

static SimDoor *door_at(SimWorld *w, uint16_t cell)
{
    for (int i = 0; i < w->door_count; i++)
        if (w->doors[i].alive && w->doors[i].cell == cell) return &w->doors[i];
    return NULL;
}

int sim_part_at(const SimWorld *w, uint16_t cell)
{
    if (!w->dog.alive) return -1;
    for (int i = 0; i < w->dog.length; i++)
        if (w->dog.chain[i] == cell) return i;
    return -1;
}

/* ------------------------------------------------------------------ */
/* Room loading                                                        */
/* ------------------------------------------------------------------ */

static int rects_strictly_overlap(float ax, float ay, float aw, float ah,
                                  float bx, float by, float bw, float bh)
{
    /* GameMaker bbox rule: touching edges do not collide. */
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

/* Cells whose probe rect strictly overlaps the placed-object bbox.  The
 * lid probe shifts the cell rect up 4px, matching the box sprite whose
 * lid pokes into the cell above (only boxes press buttons through it). */
static void compute_zone_ex(SimWorld *w, uint16_t *zone, int *count,
                            float bx, float by, float bw, float bh,
                            int with_lid)
{
    const float box_lid_dy = -4.0f;
    *count = 0;
    for (int cy = 0; cy < w->cells_h; cy++) {
        for (int cx = 0; cx < w->cells_w; cx++) {
            float rx = (float)cx * SIM_CELL;
            float ry = (float)cy * SIM_CELL;
            uint16_t cell = sim_cell_of(cx, cy);
            int hit = rects_strictly_overlap(rx, ry, SIM_CELL, SIM_CELL, bx,
                                             by, bw, bh);
            if (!hit && with_lid)
                hit = rects_strictly_overlap(rx, ry + box_lid_dy, SIM_CELL,
                                             SIM_CELL, bx, by, bw, bh);
            if (hit && *count < SIM_MAX_ZONE) zone[(*count)++] = cell;
        }
    }
}

static void compute_zone(SimWorld *w, uint16_t *zone, int *count, float bx,
                         float by, float bw, float bh)
{
    compute_zone_ex(w, zone, count, bx, by, bw, bh, 0);
}

/* Cells the dog can never enter because its shifted probe would overlap
 * the placed bbox (for 16x16 objects that is exactly the object's cell). */
static void mark_solid_footprint(SimWorld *w, float bx, float by, float bw,
                                 float bh)
{
    for (int cy = 0; cy < w->cells_h; cy++) {
        for (int cx = 0; cx < w->cells_w; cx++) {
            float rx = (float)cx * SIM_CELL;
            float ry = (float)cy * SIM_CELL;
            if (rects_strictly_overlap(rx, ry, SIM_CELL, SIM_CELL, bx, by, bw,
                                       bh))
                w->solid[sim_cell_of(cx, cy)] = 1;
        }
    }
}

static void spawn_dog(SimWorld *w, float x, float y)
{
    SimDog *dog = &w->dog;
    int cx = (int)floorf((x - 8.0f) / SIM_CELL);
    int cy = (int)floorf((y - 8.0f) / SIM_CELL);
    dog->alive = 1;
    dog->cx = cx;
    dog->cy = cy;
    dog->dir = 180; /* facing up, chain trailing below */
    dog->play = 1;
    dog->length = 5;
    dog->strain = 0;
    dog->move_timer = 0;
    dog->bark_timer = -1;
    for (int i = 0; i < SIM_MAX_CHAIN; i++) dog->chain[i] = 0;
    for (int i = 0; i < dog->length; i++) {
        dog->chain[i] = sim_cell_of(cx, cy + 1 + i);
        dog->pflag[i] = SIM_PART_BUTT;
        if (i == 0) dog->pflag[i] |= SIM_PART_FIRST | SIM_PART_LEGS;
        if (i == dog->length - 1) dog->pflag[i] = SIM_PART_LEGS;
    }
    dog->detached_cell = dog->chain[dog->length - 1];
}

/* The title room rearranges the dog into an S-curve (port of the oTitle
 * create event) and arms its idle bark. */
static void arrange_title_dog(SimWorld *w)
{
    /* head first, then the five parts of the S-curve (pixel coords in the
     * oTitle create event, snapped to cells) */
    static const int cells[6][2] = {
        { 10, 10 }, { 10, 9 }, { 9, 9 }, { 8, 9 }, { 8, 10 }, { 9, 10 }
    };
    SimDog *dog = &w->dog;
    if (!dog->alive) return;
    dog->cx = cells[0][0];
    dog->cy = cells[0][1];
    dog->dir = 0;
    dog->bark_timer = 10;
    for (int i = 0; i < dog->length; i++)
        dog->chain[i] = sim_cell_of(cells[i + 1][0], cells[i + 1][1]);
}

/* Dialogue texts, ported verbatim from the oTutorial create event. */
static const char *const TUTORIAL_TEXTS[7] = {
    "This is Longo Doggo",
    "He wants to enter his house, but he's too long so there's no room for him",
    "Apples make Longo Doggo longer",
    "Instead, pears make him shorter",
    "The house number shows how many length units you must lose",
    "If you get stuck press 'R' to retry",
    "Control Longo Doggo with the arrow keys"
};
static const float TUTORIAL_BOX_X[7] = { 103, 136, 198, 0, 144, 103, 103 };
static const float TUTORIAL_BOX_Y[7] = { 80, 56, 119, 119, 56, 80, 80 };

static const char *const LEVEL6_TEXTS[3] = {
    "This is a door",
    "It only opens once all the buttons are pressed simultaneously",
    "Buttons can be pressed either by Longo Doggo or boxes, which you can push on the sides"
};
static const float LEVEL6_BOX_X[3] = { 247, 174, 102 };
static const float LEVEL6_BOX_Y[3] = { 51, 120, 65 };

static const char *const LEVEL4_TEXTS[1] = {
    "Holes will prevent you from advancing unless you fill them with something"
};
static const float LEVEL4_BOX_X[1] = { 119 };
static const float LEVEL4_BOX_Y[1] = { 37 };

static const char *const CREDITS_TEXTS[2] = {
    "Thank you for playing! \n \n Game made by Romeu Esteve (@Romeuski) for the 'Tu juego a juicio Jam 2021' \n Using Game Maker Studio 2, freesound.org and Ableton Live 10",
    "If you enjoyed the experience please leave a comment in the itch.io page, I love feedback!"
};
static const float CREDITS_BOX_X[2] = { 151, 151 };
static const float CREDITS_BOX_Y[2] = { 37, 37 };

static void setup_dialogue(SimWorld *w, int room_index, float x, float y)
{
    SimDialogue *d = &w->dialogue;
    memset(d, 0, sizeof(*d));
    d->active = 1;
    d->index = 0;
    d->release_ticks = -1; /* not released yet */
    d->box[0].x = x;
    d->box[0].y = y;

    if (room_index == SIM_ROOM_TUTORIAL) {
        d->last = 6;
        d->gates_play = 1;
        d->base_scale = 4;
        for (int i = 0; i <= d->last; i++) {
            d->box[i].x = TUTORIAL_BOX_X[i];
            d->box[i].y = TUTORIAL_BOX_Y[i];
            d->box[i].text = TUTORIAL_TEXTS[i];
        }
        /* box 3 sits beside the first skull (skull->x - 8) */
        for (int i = 0; i < skull_count(); i++) {
            if (skull_alive(i)) {
                d->box[3].x =
                    (float)(sim_cell_x(skull_cell(i)) * SIM_CELL - 8);
                break;
            }
        }
    } else if (room_index == SIM_ROOM_LEVEL6) {
        d->last = 2;
        d->gates_play = 1;
        d->base_scale = 4;
        for (int i = 0; i <= d->last; i++) {
            d->box[i].x = LEVEL6_BOX_X[i];
            d->box[i].y = LEVEL6_BOX_Y[i];
            d->box[i].text = LEVEL6_TEXTS[i];
        }
    } else if (room_index == SIM_ROOM_LEVEL4) {
        d->last = 0;
        d->gates_play = 1;
        d->base_scale = 4;
        d->box[0].x = LEVEL4_BOX_X[0];
        d->box[0].y = LEVEL4_BOX_Y[0];
        d->box[0].text = LEVEL4_TEXTS[0];
    } else if (room_index == SIM_ROOM_CREDITS) {
        d->last = 1;
        d->gates_play = 0;
        d->base_scale = 10;
        for (int i = 0; i <= d->last; i++) {
            d->box[i].x = CREDITS_BOX_X[i];
            d->box[i].y = CREDITS_BOX_Y[i];
            d->box[i].text = CREDITS_TEXTS[i];
        }
    } else {
        d->active = 0;
    }
    if (d->gates_play) w->dog.play = 0;
}

static void load_room(SimWorld *w, int room_index)
{
    const LongoRoom *room = longo_rooms[room_index];
    float dog_x = 0, dog_y = 0;
    int have_dog = 0;
    int title = 0;
    float tutorial_x = 0, tutorial_y = 0;
    int have_tutorial = 0;

    w->room_index = room_index;
    w->room = room;
    w->cells_w = room->width / SIM_CELL;
    w->cells_h = room->height / SIM_CELL;
    if (w->cells_w > SIM_MAX_CELLS_W) w->cells_w = SIM_MAX_CELLS_W;
    if (w->cells_h > SIM_MAX_CELLS_H) w->cells_h = SIM_MAX_CELLS_H;
    w->room_loaded_tick = w->tick;

    memset(w->solid, 0, sizeof(w->solid));
    memset(&w->dog, 0, sizeof(w->dog));
    box_reset();
    hole_reset();
    items_reset();
    memset(w->buttons, 0, sizeof(w->buttons));
    w->button_count = 0;
    w->buttons_pressed = 0;
    memset(w->doors, 0, sizeof(w->doors));
    w->door_count = 0;
    house_reset();
    w->has_title = 0;
    memset(&w->dialogue, 0, sizeof(w->dialogue));

    for (int i = 0; i < room->instance_count; i++) {
        const LongoRoomInstance *p = &room->instances[i];
        switch (p->object) {
        case LONGO_OBJ_BLOCK: {
            int cx = (int)floorf(p->x / SIM_CELL);
            int cy = (int)floorf(p->y / SIM_CELL);
            if (in_bounds(w, cx, cy)) w->solid[sim_cell_of(cx, cy)] = 1;
            break;
        }
        case LONGO_OBJ_DOG:
            dog_x = p->x;
            dog_y = p->y;
            have_dog = 1;
            break;
        case LONGO_OBJ_BOX:
            box_place(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                  (int)floorf(p->y / SIM_CELL)));
            break;
        case LONGO_OBJ_HOLE:
            hole_place(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                   (int)floorf(p->y / SIM_CELL)));
            break;
        case LONGO_OBJ_APPLE:
            apple_place(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                    (int)floorf(p->y / SIM_CELL)));
            break;
        case LONGO_OBJ_SKULL:
            skull_place(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                    (int)floorf(p->y / SIM_CELL)));
            break;
        case LONGO_OBJ_BUTTON:
            if (w->button_count < SIM_MAX_BUTTONS) {
                SimButton *b = &w->buttons[w->button_count++];
                b->alive = 1;
                b->pressed = 0;
                compute_zone(w, b->zone, &b->zone_count, p->x, p->y, 16, 16);
                compute_zone_ex(w, b->box_zone, &b->box_zone_count, p->x,
                                p->y, 16, 16, 1);
            }
            break;
        case LONGO_OBJ_DOOR:
            if (w->door_count < SIM_MAX_DOORS) {
                SimDoor *d = &w->doors[w->door_count++];
                d->alive = 1;
                d->open = 0;
                d->open_timer = 0;
                d->cell = sim_cell_of((int)floorf(p->x / SIM_CELL),
                                      (int)floorf(p->y / SIM_CELL));
            }
            break;
        case LONGO_OBJ_GOAL: {
            /* placed directly (title/tutorial rooms); oGoal's sprite is the
             * 64x64 sprHouse with origin (32, 64) */
            float gx = p->x - 32;
            float gy = p->y - 64;
            house_place_goal(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                         (int)floorf((p->y - 32) / SIM_CELL)));
            mark_solid_footprint(w, gx, gy, 64, 64);
            break;
        }
        case LONGO_OBJ_HOUSESPAWNER: {
            /* oHouseSpawner step: goal at (x+8, y+16), win at (x-8, y+16)
             * with xscale 2 (a 32x32 bbox) */
            float gx = p->x + 8;
            float gy = p->y + 16;
            float wx = p->x - 8;
            float wy = p->y + 16;
            house_place_goal(sim_cell_of((int)floorf(gx / SIM_CELL),
                                         (int)floorf((gy - 32) / SIM_CELL)));
            mark_solid_footprint(w, gx - 32, gy - 64, 64, 64);
            {
                uint16_t zone[HOUSE_WIN_ZONE_MAX];
                int count;
                compute_zone(w, zone, &count, wx, wy, 32, 32);
                house_place_win_zone(zone, count);
            }
            break;
        }
        case LONGO_OBJ_WIN: {
            uint16_t zone[HOUSE_WIN_ZONE_MAX];
            int count;
            compute_zone(w, zone, &count, p->x, p->y, 16 * p->xscale,
                         16 * p->yscale);
            house_place_win_zone(zone, count);
            break;
        }
        case LONGO_OBJ_TITLE:
            title = 1;
            events_sound(SND_PLACEHOLDER, 1);
            break;
        case LONGO_OBJ_TUTORIAL:
            tutorial_x = p->x;
            tutorial_y = p->y;
            have_tutorial = 1;
            break;
        default:
            /* FLOWER, SHADOWS, BUTTERFLY, BLOOM, POSTEFFECTS: presentation
             * decor.  MOUSE/DOGSPAWNER: the editor room is dropped. */
            break;
        }
    }

    w->has_title = title;
    if (have_dog) spawn_dog(w, dog_x, dog_y);
    if (title) arrange_title_dog(w);
    if (have_tutorial) setup_dialogue(w, room_index, tutorial_x, tutorial_y);
    /* oGoalUp create defaults remain to 1; the first house_tick recomputes
     * it to dog.length - 2.  Initialising here prevents the win from
     * arming on the load tick itself (house_reset pre-seeds it). */
}

/* ------------------------------------------------------------------ */
/* Buttons, doors, goal (step + draw-mutation ports)                   */
/* ------------------------------------------------------------------ */

static void update_buttons(SimWorld *w)
{
    for (int i = 0; i < w->button_count; i++) {
        SimButton *b = &w->buttons[i];
        int pressed = 0;
        /* boxes press through the lid zone; everything else through the
         * body zone */
        for (int z = 0; z < b->box_zone_count && !pressed; z++) {
            uint16_t cell = b->box_zone[z];
            if (cell_solid(w, cell)) continue;
            if (box_index_at(cell) >= 0) pressed = 1;
        }
        for (int z = 0; z < b->zone_count && !pressed; z++) {
            uint16_t cell = b->zone[z];
            if (cell_solid(w, cell)) continue;
            {
                int hi = hole_index_at(cell);
                if ((hi >= 0 && !hole_is_full(hi)) || box_index_at(cell) >= 0 ||
                    door_at(w, cell))
                    pressed = 1;
            }
            if (sim_part_at(w, cell) >= 0) pressed = 1;
            if (w->dog.alive && sim_cell_of(w->dog.cx, w->dog.cy) == cell)
                pressed = 1;
        }
        if (pressed && !b->pressed) {
            events_sound(SND_BUTTON, 0);
            w->buttons_pressed++;
        } else if (!pressed && b->pressed) {
            events_sound(SND_WRONG, 0);
            w->buttons_pressed--;
        }
        b->pressed = pressed;
    }
}

/* The open animation eases scale 1 -> 1.2 at 0.1/tick and poofs past 1.15
 * (14 ticks); the cell stays solid for that duration, like the original. */
#define SIM_DOOR_OPEN_TICKS 14

static void update_doors(SimWorld *w)
{
    for (int i = 0; i < w->door_count; i++) {
        SimDoor *d = &w->doors[i];
        if (!d->alive) continue;
        if (!d->open && w->buttons_pressed == w->button_count) {
            d->open = 1;
            d->open_timer = SIM_DOOR_OPEN_TICKS;
        }
        if (d->open) {
            if (--d->open_timer <= 0) {
                d->alive = 0;
                events_fx(FX_SMOKE_BURST,
                          (float)(sim_cell_x(d->cell) * SIM_CELL),
                          (float)(sim_cell_y(d->cell) * SIM_CELL), 0, 0, 0, 7,
                          0);
                events_sound(SND_POOF, 0);
            }
        }
    }
}

static void update_goal(SimWorld *w)
{
    house_tick(w->dog.alive ? w->dog.length : -1);
}

/* ------------------------------------------------------------------ */
/* Meta state machines (title, dialogue, transition)                   */
/* ------------------------------------------------------------------ */

static void update_title(SimWorld *w, const SimInput *input)
{
    if (!w->has_title || !input->pressed_any) return;
    w->trans.active = 1;
    w->trans.next_lvl = 1;
    w->trans.open_transition = 1;
}

/* oTutorial Draw event port; the box scale easing is presentation-side. */
static void update_dialogue(SimWorld *w, const SimInput *input)
{
    SimDialogue *d = &w->dialogue;
    if (!d->active) return;
    if (d->release_ticks >= 0) {
        if (++d->release_ticks >= SIM_DIALOGUE_SHRINK_TICKS) d->active = 0;
        return;
    }
    if ((input->pressed_space || input->pressed_enter || input->pressed_e) &&
        !w->trans.close_transition) {
        if (d->index < d->last) {
            d->index++;
        } else {
            d->release_ticks = 0;
            if (d->gates_play) w->dog.play = 1;
        }
    }
}

/* oTransition Draw event port, verbatim (it is the one state machine the
 * sim keeps its animated floats for; the renderer reads them directly). */
static void update_transition(SimWorld *w)
{
    SimTransition *t = &w->trans;
    if (!t->active) return;
    if (t->open_transition) {
        if (t->x >= -20.0f) {
            t->x = f_lerp(t->x, -31.0f, 0.04f);
        } else {
            t->x = 304.0f;
            t->close_transition = 1;
            if (t->retry) {
                sim_room_restart(w);
                t->retry = 0;
            } else if (t->next_lvl) {
                sim_room_goto_next(w);
                t->next_lvl = 0;
            }
            t->open_transition = 0;
        }
        if (t->room_num <= 7) t->text_y = f_lerp(t->text_y, 208.0f / 2 + 4, 0.05f);
    } else if (t->close_transition) {
        /* GML ran this lerp four times per frame (208/64 passes). */
        for (int i = 0; i < 4; i++) {
            if (t->x >= -63.0f) {
                t->x = f_lerp(t->x, -64.0f, 0.02f);
            } else {
                t->close_transition = 0;
            }
        }
        if (t->room_num <= 7) t->text_y = f_lerp(t->text_y, 208.0f + 16, 0.16f);
    } else {
        t->x = 304.0f;
        t->text_y = -16.0f;
    }
}

/* ------------------------------------------------------------------ */
/* Tick                                                                */
/* ------------------------------------------------------------------ */

void sim_tick(SimWorld *w, const SimInput *input)
{
    w->tick++;
    events_clear();

    /* 1. dog step + pickups (oDog Step, then collision events) */
    sim_dog_step(w, input);

    /* 2. world object steps + draw-mutation ports */
    update_buttons(w);
    update_doors(w);
    update_goal(w);

    /* 3. title -> transition request */
    if (w->room_loaded_tick != w->tick) update_title(w, input);

    /* 4. transition FSM (may reload the room mid-tick, like the original
     * persistent instance) */
    update_transition(w);

    /* 5. dialogue (runs on the room loaded this tick? the original skipped
     * instances born this tick, so skip if the room just changed) */
    if (w->room_loaded_tick != w->tick) update_dialogue(w, input);
}

/* ------------------------------------------------------------------ */
/* Room management                                                     */
/* ------------------------------------------------------------------ */

void sim_room_goto(SimWorld *w, int room_index)
{
    if (room_index < 0 || room_index >= SIM_ROOM_COUNT) return;
    load_room(w, room_index);
}

void sim_room_goto_next(SimWorld *w)
{
    sim_room_goto(w, w->room_index + 1); /* out of range keeps the room */
}

void sim_room_restart(SimWorld *w) { sim_room_goto(w, w->room_index); }

void sim_init(unsigned int seed)
{
    SimWorld *w = &game_world;
    memset(w, 0, sizeof(*w));
    w->rng = seed ? seed : 0x1234u;
    w->tick = 1;
    /* oTransition create values; it persists across rooms like the
     * original persistent instance (placed only in the title room). */
    w->trans.active = 1;
    w->trans.x = 304.0f;
    w->trans.text_y = -16.0f;
    w->trans.room_num = 1;
    load_room(w, SIM_ROOM_TITLE);
}
