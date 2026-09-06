/*
 * Simulation core: world state, room loading (level_data objects ->
 * cells + entities), and the tick orchestrator.  Movement rules live in
 * dog.c.  Rules are expressed as cell lookups over the 16px grid (see
 * docs/architecture.md).
 */
#include "world.h"
#include "../room_tiles.h"

#include "../objects/button.h"
#include "../objects/box.h"
#include "../objects/door.h"
#include "../objects/dog.h"
#include "../objects/dialogue.h"
#include "../objects/house.h"
#include "../objects/title.h"
#include "../objects/transition.h"
#include "../objects/items.h"
#include "../objects/hole.h"
#include "events.h"
#include "view.h"

#include "../objects/butterfly.h"
#include "../objects/flower.h"
#include "../objects/fx.h"
#include "solid.h"
#include "undo.h"
#include "rng.h"

#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

int sim_cell_x(uint16_t cell) { return cell % SIM_MAX_CELLS_W; }
int sim_cell_y(uint16_t cell) { return cell / SIM_MAX_CELLS_W; }
uint16_t sim_cell_of(int cx, int cy) { return SIM_CELL_INDEX(cx, cy); }

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

float world_random(float max)
{
    Rng rng = { game_world.rng };
    float value = rng_float(&rng, max);
    game_world.rng = rng.state;
    return value;
}

float world_random_range(float lo, float hi)
{
    return lo + world_random(hi - lo);
}

/* ------------------------------------------------------------------ */
/* Entity lookups (linear scans; the counts are tiny)                  */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Room loading                                                        */
/* ------------------------------------------------------------------ */

static int rects_strictly_overlap(float ax, float ay, float aw, float ah,
                                  float bx, float by, float bw, float bh)
{
    /* strictly overlapping rects only: touching edges do not collide. */
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

/* Cells whose 16px probe rect strictly overlaps the placed-object rect.
 * A box taller than one cell visually overlaps the button in the cell
 * below, but counting that reads as a false press in play, so boxes only
 * press from the button's own cell here. */
static void compute_zone(SimWorld *w, uint16_t *zone, int *count, float bx,
                         float by, float bw, float bh)
{
    *count = 0;
    for (int cy = 0; cy < w->cells_h; cy++) {
        for (int cx = 0; cx < w->cells_w; cx++) {
            float rx = (float)cx * SIM_CELL;
            float ry = (float)cy * SIM_CELL;
            uint16_t cell = sim_cell_of(cx, cy);
            if (rects_strictly_overlap(rx, ry, SIM_CELL, SIM_CELL, bx, by, bw,
                                       bh) &&
                *count < BUTTON_ZONE_MAX)
                zone[(*count)++] = cell;
        }
    }
}

/* Cells whose 16px probe rect strictly overlaps the footprint get `kind`
 * (touching edges do not collide).  This is the one footprint ->
 * solid-map rasterizer: every object that occupies area is loaded
 * through it, scaled or not, so all probes see the same map. */
static void mark_footprint(SimWorld *w, float bx, float by, float bw,
                           float bh, SolidKind kind)
{
    for (int cy = 0; cy < w->cells_h; cy++) {
        for (int cx = 0; cx < w->cells_w; cx++) {
            float rx = (float)cx * SIM_CELL;
            float ry = (float)cy * SIM_CELL;
            if (rects_strictly_overlap(rx, ry, SIM_CELL, SIM_CELL, bx, by, bw,
                                       bh))
                solid_place(sim_cell_of(cx, cy), kind, 0);
        }
    }
}

/* The tutorial dialogue texts. */
static void load_room(SimWorld *w, int room_index)
{
    const LongoRoom *room = longo_rooms[room_index];
    float dog_x = 0, dog_y = 0;
    int have_dog = 0;
    int title = 0;
    int have_tutorial = 0;

    w->room_index = room_index;
    w->room = room;
    w->cells_w = room->width / SIM_CELL;
    w->cells_h = room->height / SIM_CELL;
    if (w->cells_w > SIM_MAX_CELLS_W) w->cells_w = SIM_MAX_CELLS_W;
    if (w->cells_h > SIM_MAX_CELLS_H) w->cells_h = SIM_MAX_CELLS_H;
    w->room_loaded_tick = w->tick;

    solid_reset();
    dog_reset();
    box_reset();
    hole_reset();
    items_reset();
    house_reset();
    button_reset();
    door_reset();
    title_reset();
    dialogue_reset();
    flower_reset();
    butterfly_reset();
    fx_reset(); /* smoke/bark/popups do not survive a room load */
    undo_reset(); /* neither does the move history */
    w->shadows_present = 0;

    for (int i = 0; i < room->object_count; i++) {
        const LongoRoomObject *p = &room->objects[i];
        switch (p->object) {
        case LONGO_OBJ_BLOCK:
            /* the wall object is the wall collider; rooms stamp it scaled
             * (xscale * 16px wide, yscale * 16px tall).  The old
             * single-cell load
             * left most of every scaled wall walkable. */
            mark_footprint(w, p->x, p->y, 16.0f * p->xscale,
                           16.0f * p->yscale, SOLID_WALL);
            break;
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
        case LONGO_OBJ_BUTTON: {
            uint16_t zone[BUTTON_ZONE_MAX];
            int zone_count;
            compute_zone(w, zone, &zone_count, p->x, p->y, 16, 16);
            button_place(zone, zone_count);
            break;
        }
        case LONGO_OBJ_DOOR:
            door_place(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                   (int)floorf(p->y / SIM_CELL)));
            break;
        case LONGO_OBJ_GOAL: {
            /* placed directly (title/tutorial rooms); the goal sprite is
             * the 64x64 house with origin (32, 64).  The solid mask follows
             * the house walls (sprite x 9..54 -> 48px centred on the
             * anchor, from the stored cell down); the roof and eaves
             * overhang stay background so the entrance stays reachable. */
            float gx = p->x - 24;
            float gy = p->y - 32;
            house_place_goal(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                         (int)floorf((p->y - 32) / SIM_CELL)));
            mark_footprint(w, gx, gy, 48, 32, SOLID_GOAL);
            break;
        }
        case LONGO_OBJ_HOUSESPAWNER: {
            /* house spawner: goal at (x+8, y+16), win at (x-8, y+16)
             * with xscale 2 (a 32x32 footprint); wall-width mask like the
             * directly placed goal */
            float gx = p->x + 8;
            float gy = p->y + 16;
            float wx = p->x - 8;
            float wy = p->y + 16;
            house_place_goal(sim_cell_of((int)floorf(gx / SIM_CELL),
                                         (int)floorf((gy - 32) / SIM_CELL)));
            mark_footprint(w, gx - 24, gy - 32, 48, 32, SOLID_GOAL);
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
            title_place();
            break;
        case LONGO_OBJ_TUTORIAL:
            have_tutorial = 1;
            break;
        case LONGO_OBJ_FLOWER:
            flower_place(p->x, p->y);
            break;
        case LONGO_OBJ_BUTTERFLY:
            butterfly_place(p->x, p->y);
            break;
        case LONGO_OBJ_SHADOWS:
            w->shadows_present = 1;
            break;
        default:
            /* POSTEFFECTS: render-side passes.  MOUSE/DOGSPAWNER:
             * the editor room is dropped. */
            break;
        }
    }

    if (have_dog) dog_place(dog_x, dog_y);
    if (title) dog_title_arrangement();
    if (have_tutorial) dialogue_start(room_index);
    /* house_tick recomputes remain to dog.length - 2 on its first tick;
     * the pre-seeded 1 prevents the win from arming on the load tick. */
}

/* ------------------------------------------------------------------ */
/* Buttons, doors, goal                                                */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Tick                                                                */
/* ------------------------------------------------------------------ */

void sim_tick(SimWorld *w, const SimInput *input)
{
    if (input) w->input = *input;
    else memset(&w->input, 0, sizeof(w->input));
    w->tick++;
    events_clear();

    /* 0. undo press: restore the pre-step board and consume the tick,
     * so the restored state is not also stepped this frame */
    if (undo_tick(&w->input)) return;

    /* 1. dog step + pickups */
    dog_tick(&w->input);

    /* 2. world object steps */
    button_tick();
    door_tick(button_all_pressed());
    house_tick(dog_alive() ? dog_length() : -1);

    /* 3. title -> transition request */
    if (w->room_loaded_tick != w->tick) title_tick(&w->input);

    /* 4. transition FSM (may reload the room mid-tick) */
    transition_tick(w);

    /* 5. dialogue (skipped on the tick a room loads, so a fresh room's
     * dialogue does not advance immediately) */
    if (w->room_loaded_tick != w->tick) dialogue_tick(&w->input);
}

/* ------------------------------------------------------------------ */
/* View orchestration                                                  */
/* ------------------------------------------------------------------ */

void world_view_tick(const SimInput *input)
{
    dog_view_tick();
    box_view_tick();
    door_view_tick();
    house_view_tick();
    dialogue_view_tick();
    butterfly_tick(input);
    fx_tick();
}

void world_draw(void)
{
    view_begin_frame();

    /* shadow surface: everything casts its shadow first */
    view_layer(VIEW_SHADOW);
    dog_draw(1);
    box_draw(1);
    door_draw(1);
    button_draw(1);
    items_draw(1);
    house_draw(1);
    butterfly_draw(1);
    title_draw(1);

    /* application surface: background, tile layers, shadow composite and
     * the entities in depth order */
    view_layer(VIEW_WORLD);
    view_tile_layers(VIEW_DEPTH_TILES, 0); /* sprTile background */
    const LongoRoomTileMap *tiles = room_tiles_for(game_world.room);
    if (tiles != NULL)
        view_tile_layers(tiles->tiles_3.depth, 1);
    if (game_world.shadows_present && dog_alive())
        view_shadow_composite(210);
    dog_draw(0);
    box_draw(0);
    door_draw(0);
    hole_draw();
    button_draw(0);
    items_draw(0);
    house_draw(0);
    flower_draw(0);
    butterfly_draw(0);
    title_draw(0);
    fx_draw();

    /* GUI surface: dialogue boxes and level wipes */
    view_layer(VIEW_GUI);
    dialogue_draw();
    transition_draw();
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
    /* The transition outlives room loads (its object is placed only in
     * the title room but must keep wiping across rooms). */
    transition_reset();
    load_room(w, SIM_ROOM_TITLE);
}
