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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

int sim_cell_x(uint16_t cell) { return cell % SIM_MAX_CELLS_W; }
int sim_cell_y(uint16_t cell) { return cell / SIM_MAX_CELLS_W; }
uint16_t sim_cell_of(int cx, int cy) { return SIM_CELL_INDEX(cx, cy); }

/* The one SimWorld.  There is no multi-world support: the functions
 * that take a SimWorld * all receive &game_world (the tick order passes
 * it along to document data flow), and world_ptr() hands the same
 * instance to scripts and the front-end.  sim_init() is the single
 * owner of this instance's lifetime: it is the only place that
 * reinitializes it from scratch (a NEW GAME). */
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

/* One loading policy: every catalog room is validated before its
 * objects are loaded, so malformed data fails loudly with the room's
 * name instead of being silently clipped or dropped on the floor.
 *
 * The policy distinguishes intentional clips from malformed content:
 *   - offscreen placements (negative or oversized coordinates, the
 *     border walls stamped past the play area, decoration outside the
 *     room rect) are authored decoration; the loader intersects them
 *     with the sim grid by design and the validator only counts them;
 *   - dimensions that miss the sim grid (not a whole multiple of
 *     SIM_CELL, or larger than SIM_MAX_CELLS_W/H) are a hard failure;
 *   - functional objects (items per kind, buttons, doors, boxes,
 *     holes, zone areas) beyond the entity pool capacities, and a
 *     second dog/goal overwriting the singletons, are a hard failure.
 * Every shipped room passes; these only fire on future malformed data.
 * longo_room_validate() reports; load_room turns violations into an
 * abort with the room's name. */
typedef struct RoomLoadCounts {
    int apples, pears, boxes, holes, buttons, doors;
    int dogs, goals, wins; /* dog, house goal and win zone are singletons */
} RoomLoadCounts;

static void validation_fail(const LongoRoom *room, LongoRoomValidation *out,
                            const char *what)
{
    out->violations++;
    if (out->first_violation[0] == '\0')
        snprintf(out->first_violation, sizeof(out->first_violation), "%s: %s",
                 room->name ? room->name : "unnamed room", what);
}

static void validation_fail_capacity(const LongoRoom *room,
                                     LongoRoomValidation *out, int count,
                                     int cap, const char *what)
{
    char detail[160];
    snprintf(detail, sizeof(detail), "%d %s exceed capacity %d", count, what,
             cap);
    validation_fail(room, out, detail);
}

/* The one "is this placement's cell inside the room grid" test, shared
 * by the validator (which counts the placements that miss as authored
 * offscreen decoration) and the loader (which skips exactly those), so
 * the two notions can never diverge.  Without the shared bound, a
 * negative or oversized pixel coordinate reaches the solid map unchecked
 * or wraps through its SIM_MAX_CELLS_W stride onto a playable cell. */
static bool object_cell_in_grid(const LongoRoom *room, float x, float y,
                                int *out_cx, int *out_cy)
{
    if (!(x >= 0 && y >= 0 && x < room->width && y < room->height))
        return false;
    int cx = (int)floorf(x / SIM_CELL);
    int cy = (int)floorf(y / SIM_CELL);
    if (out_cx) *out_cx = cx;
    if (out_cy) *out_cy = cy;
    return cx >= 0 && cy >= 0 && cx < room->width / SIM_CELL &&
           cy < room->height / SIM_CELL;
}

bool longo_room_data_validate(const LongoRoom *room, LongoRoomValidation *out)
{
    LongoRoomValidation report;
    int cells_w, cells_h;
    RoomLoadCounts counts = {0};

    memset(&report, 0, sizeof(report));
    if (out) *out = report;
    if (room == NULL) {
        snprintf(report.first_violation, sizeof(report.first_violation),
                 "room table is NULL");
        report.violations++;
        if (out) *out = report;
        return false;
    }

    if (room->object_count < 0 || (room->object_count > 0 && !room->objects)) {
        validation_fail(room, &report, "invalid object table or count");
        if (out) *out = report;
        return false;
    }

    /* dimensions: the sim grid must fit the room exactly */
    if (room->width % SIM_CELL != 0 || room->height % SIM_CELL != 0) {
        validation_fail(room, &report, "pixel size not a multiple of "
                                       "SIM_CELL");
    }
    cells_w = room->width / SIM_CELL;
    cells_h = room->height / SIM_CELL;
    if (cells_w > SIM_MAX_CELLS_W || cells_h > SIM_MAX_CELLS_H) {
        char detail[160];
        snprintf(detail, sizeof(detail), "%dx%d cells exceed sim grid %dx%d",
                 cells_w, cells_h, SIM_MAX_CELLS_W, SIM_MAX_CELLS_H);
        validation_fail(room, &report, detail);
    }
    /* a zero or negative dimension carries no usable grid: every cell
     * computation on it would index the solid map from outside */
    if (cells_w <= 0 || cells_h <= 0) {
        char detail[160];
        snprintf(detail, sizeof(detail), "room is %dx%d cells, must be > 0",
                 cells_w, cells_h);
        validation_fail(room, &report, detail);
    }

    for (int i = 0; i < room->object_count; i++) {
        const LongoRoomObject *p = &room->objects[i];
        if (!isfinite(p->x) || !isfinite(p->y) ||
            !isfinite(p->xscale) || !isfinite(p->yscale)) {
            validation_fail(room, &report, "object coordinates and scales must be finite");
            continue;
        }
        /* ids outside the LongoObj enum (level_data.h) are malformed
         * data, never content: an unknown id must fail loudly instead of
         * falling through the loader's ignore cases */
        if (p->object < LONGO_OBJ_PEAR || p->object > LONGO_OBJ_ONE) {
            char detail[160];
            snprintf(detail, sizeof(detail), "object %d has unknown id %u", i,
                     (unsigned)p->object);
            validation_fail(room, &report, detail);
            continue;
        }
        switch (p->object) {
        case LONGO_OBJ_BOX:
        case LONGO_OBJ_HOLE:
        case LONGO_OBJ_APPLE:
        case LONGO_OBJ_PEAR:
        case LONGO_OBJ_BUTTON:
        case LONGO_OBJ_DOOR:
        case LONGO_OBJ_HOUSESPAWNER:
        case LONGO_OBJ_GOAL:
        case LONGO_OBJ_WIN: {
            /* functional cell objects: off-grid placements are authored
             * decoration (the loader leaves them unloaded) and only the
             * in-grid ones claim pool capacity */
            int cx, cy;
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) {
                report.offscreen++;
                break;
            }
            switch (p->object) {
            case LONGO_OBJ_APPLE: counts.apples++; break;
            case LONGO_OBJ_PEAR: counts.pears++; break;
            case LONGO_OBJ_BOX: counts.boxes++; break;
            case LONGO_OBJ_HOLE: counts.holes++; break;
            case LONGO_OBJ_BUTTON: counts.buttons++; break;
            case LONGO_OBJ_DOOR: counts.doors++; break;
            case LONGO_OBJ_GOAL:
            case LONGO_OBJ_HOUSESPAWNER: counts.goals++; break;
            case LONGO_OBJ_WIN: counts.wins++; break;
            default: break;
            }
            if (p->object == LONGO_OBJ_BUTTON) {
                /* a 16px button zone strictly overlaps at most 2x2
                 * probe cells */
                if (4 > BUTTON_ZONE_MAX)
                    validation_fail_capacity(room, &report, 4,
                                             BUTTON_ZONE_MAX, "button zone");
            } else if (p->object == LONGO_OBJ_WIN ||
                       p->object == LONGO_OBJ_HOUSESPAWNER) {
                /* win-zone areas: worst-case strict overlap of the rect
                 * with the 16px probe grid must fit the zone pool (the
                 * spawner's zone is a fixed 32x32; a GOAL places no
                 * zone of its own) */
                double zw = (p->object == LONGO_OBJ_WIN) ? 16.0 * p->xscale : 32.0;
                double zh = (p->object == LONGO_OBJ_WIN) ? 16.0 * p->yscale : 32.0;
                double max_cells = (floor(fabs(zw) / SIM_CELL) + 1) *
                                   (floor(fabs(zh) / SIM_CELL) + 1);
                if (max_cells > HOUSE_WIN_ZONE_MAX) {
                    char detail[160];
                    snprintf(detail, sizeof(detail),
                             "win zone %.0fx%.0f px may span %.0f cells, capacity "
                             "%d",
                             zw, zh, max_cells, HOUSE_WIN_ZONE_MAX);
                    validation_fail(room, &report, detail);
                }
            }
            break;
        }
        case LONGO_OBJ_BLOCK: {
            /* walls stamp by intersection: only a block entirely off the
             * room rect is offscreen decoration */
            float x1 = p->x + 16.0f * p->xscale;
            float y1 = p->y + 16.0f * p->yscale;
            if (x1 <= 0.0f || y1 <= 0.0f || p->x >= (float)room->width ||
                p->y >= (float)room->height)
                report.offscreen++;
            break;
        }
        case LONGO_OBJ_FLOWER:
            if (p->x < 0.0f || p->y < 0.0f ||
                p->x >= (float)room->width || p->y >= (float)room->height)
                report.offscreen++;
            break;
        case LONGO_OBJ_DOG: {
            /* Placement uses an 8px origin and extends the initial body
             * downward. Check floats before dog_place converts to cells. */
            float cx = floorf((p->x - 8.0f) / SIM_CELL);
            float cy = floorf((p->y - 8.0f) / SIM_CELL);
            if (!isfinite(cx) || !isfinite(cy) || cx < 0 || cy < 0 ||
                cx >= cells_w || cy + DOG_INITIAL_LENGTH >= cells_h)
                validation_fail(room, &report, "dog spawn footprint exceeds room grid");
            counts.dogs++;
            break;
        }
        default:
            /* ids the loader deliberately ignores (its default case
             * mirrors this list): LONGO_OBJ_HIDDEN_BLOCK,
             * LONGO_OBJ_TRANSITION, LONGO_OBJ_MOUSE, LONGO_OBJ_GOALUP,
             * LONGO_OBJ_PAR_MODULE, LONGO_OBJ_POSTEFFECTS,
             * LONGO_OBJ_DOGPART, LONGO_OBJ_SMOKE, LONGO_OBJ_BARK,
             * LONGO_OBJ_DOGSPAWNER, LONGO_OBJ_ONE — render-side passes
             * and editor-only objects carry no capacity */
            break; /* cosmetics and editor-only objects carry no capacity */
        }
    }

    if (counts.apples > ITEMS_MAX)
        validation_fail_capacity(room, &report, counts.apples, ITEMS_MAX,
                                 "apples");
    if (counts.pears > ITEMS_MAX)
        validation_fail_capacity(room, &report, counts.pears, ITEMS_MAX,
                                 "pears");
    if (counts.boxes > BOX_MAX)
        validation_fail_capacity(room, &report, counts.boxes, BOX_MAX, "boxes");
    if (counts.holes > HOLE_MAX)
        validation_fail_capacity(room, &report, counts.holes, HOLE_MAX,
                                 "holes");
    if (counts.buttons > BUTTON_MAX)
        validation_fail_capacity(room, &report, counts.buttons, BUTTON_MAX,
                                 "buttons");
    if (counts.doors > DOOR_MAX)
        validation_fail_capacity(room, &report, counts.doors, DOOR_MAX,
                                 "doors");
    if (counts.dogs > 1)
        validation_fail_capacity(room, &report, counts.dogs, 1, "dog spawns");
    if (counts.goals > 1)
        validation_fail_capacity(room, &report, counts.goals, 1,
                                 "house goals");
    if (counts.wins > 1)
        validation_fail_capacity(room, &report, counts.wins, 1,
                                 "house win zones");

    if (out) *out = report;
    return report.violations == 0;
}

/* Catalog entry: the table check plus the catalog-level requirement
 * that every in-play room owns its tile map entry (the lookup lives in
 * room_tiles.c and is reached through its query function). */
bool longo_room_validate(int room_index, LongoRoomValidation *out)
{
    LongoRoomValidation report;
    const LongoRoom *room = longo_room(room_index);

    memset(&report, 0, sizeof(report));
    if (out) *out = report;
    if (room == NULL) {
        snprintf(report.first_violation, sizeof(report.first_violation),
                 "catalog index %d out of range", room_index);
        report.violations++;
        if (out) *out = report;
        return false;
    }
    if (!longo_room_data_validate(room, &report)) {
        if (out) *out = report;
        return false;
    }
    if (room_index < longo_room_play_count() && room_tiles_for(room) == NULL) {
        validation_fail(room, &report, "in-play room has no tile map");
        if (out) *out = report;
        return false;
    }
    if (out) *out = report;
    return report.violations == 0;
}

/* The loader's precondition: never load a room the validator rejects. */
static void validate_room_or_abort(int room_index)
{
    LongoRoomValidation report;
    if (longo_room_validate(room_index, &report)) return;
    fprintf(stderr, "room data: %s\n", report.first_violation);
    fprintf(stderr, "room data: fix the authored tables in level_data.c\n");
    abort();
}

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

/* A room load (the next room, a retry, or the new-game title load)
 * resets the room-scoped state only: the solid map, dog, box, hole,
 * items, house, button, door, title, dialogue, flower, butterfly, fx
 * pools, the undo history and shadows_present.
 *
 * What deliberately survives a room load:
 *   - the transition wipe (phase + pending action + level label), so
 *     the wipe keeps running across the mid-wipe room change;
 *   - the view module's animation clocks (core/view.c statics) keep
 *     running; only sim_init() restarts them (view_reset);
 *   - w->tick and the gameplay rng stream in w->rng.
 *
 * The cosmetic rng streams (fx, butterflies) are per-object by design
 * (see core/rng.h) and reseed with their objects here, so a room load
 * replays the same cosmetic scatter; they only ever feed visuals.
 *
 * The tutorial dialogue texts. */
static void load_room(SimWorld *w, int room_index)
{
    const LongoRoom *room = longo_room(room_index);
    float dog_x = 0, dog_y = 0;
    int have_dog = 0;
    int title = 0;
    int have_tutorial = 0;

    if (room == NULL) return;
    /* the one load policy: malformed data never reaches the loaders */
    validate_room_or_abort(room_index);
    w->room_index = room_index;
    w->room = room;
    /* the validator guaranteed the exact fit (whole SIM_CELL multiples
     * within SIM_MAX_CELLS_W/H), so no clipping here */
    w->cells_w = room->width / SIM_CELL;
    w->cells_h = room->height / SIM_CELL;
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
        /* every cell-based functional placement first resolves its cell
         * through the same in-grid test the validator counts with: a
         * placement outside the room grid is authored decoration and is
         * skipped, never wrapped onto a playable cell */
        int cx, cy;
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
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            box_place(sim_cell_of(cx, cy));
            break;
        case LONGO_OBJ_HOLE:
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            hole_place(sim_cell_of(cx, cy));
            break;
        case LONGO_OBJ_APPLE:
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            apple_place(sim_cell_of(cx, cy));
            break;
        case LONGO_OBJ_PEAR:
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            pear_place(sim_cell_of(cx, cy));
            break;
        case LONGO_OBJ_BUTTON: {
            uint16_t zone[BUTTON_ZONE_MAX];
            int zone_count;
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            compute_zone(w, zone, &zone_count, p->x, p->y, 16, 16);
            button_place(zone, zone_count);
            break;
        }
        case LONGO_OBJ_DOOR:
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            door_place(sim_cell_of(cx, cy));
            break;
        case LONGO_OBJ_GOAL: {
            /* placed directly (title/tutorial rooms); the goal sprite is
             * the 64x64 house with origin (32, 64).  The solid mask follows
             * the house walls (sprite x 9..54 -> 48px centred on the
             * anchor, from the stored cell down); the roof and eaves
             * overhang stay background so the entrance stays reachable. */
            float gx, gy;
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            gx = p->x - 24;
            gy = p->y - 32;
            house_place_goal(sim_cell_of((int)floorf(p->x / SIM_CELL),
                                         (int)floorf((p->y - 32) / SIM_CELL)));
            mark_footprint(w, gx, gy, 48, 32, SOLID_GOAL);
            break;
        }
        case LONGO_OBJ_HOUSESPAWNER: {
            /* house spawner: goal at (x+8, y+16), win at (x-8, y+16)
             * with xscale 2 (a 32x32 footprint); wall-width mask like the
             * directly placed goal */
            float gx, gy, wx, wy;
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
            gx = p->x + 8;
            gy = p->y + 16;
            wx = p->x - 8;
            wy = p->y + 16;
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
            if (!object_cell_in_grid(room, p->x, p->y, &cx, &cy)) break;
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
    if (have_tutorial) dialogue_start(room);
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
    /* one rules tick is one 60 Hz step: it owns the animation clocks
     * (view_update), so drawing never advances time */
    view_update();
    events_clear();

    /* 0. undo press: restore the pre-step board; the tick then runs on
     * over it, so buttons/doors/the house re-derive in the same frame */
    undo_tick(&w->input);

    /* 1. room-control input: R retries the room unless a wipe is
     * closing (a closing wipe's pending room change wins).  Room-flow
     * policy lives here with the tick order, not in an object script. */
    if (w->input.pressed_r && !transition_closing())
        transition_request_retry();

    /* 2. dog step + pickups */
    dog_tick(&w->input);

    /* 3. world object steps */
    button_tick();
    door_tick(button_all_pressed());
    house_tick(dog_alive() ? dog_length() : -1);

    /* 4. title -> transition request */
    if (w->room_loaded_tick != w->tick) title_tick(&w->input);

    /* 5. transition FSM (may reload the room mid-tick) */
    transition_tick(w);

    /* 6. dialogue (skipped on the tick a room loads, so a fresh room's
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
/* Fixed-step schedule                                                 */
/* ------------------------------------------------------------------ */

/* One update = one rules tick (which steps the view clocks) + the view
 * pass that consumes the tick's fx events + sound delivery.  The
 * schedule state is front-end scheduling, not game state; sim_init
 * resets it so a new game starts the clock over. */
static double sched_acc_ms;
static SimInput sched_held;
static int sched_held_valid;

void sim_frame(double dt_ms, const SimInput *input,
               SimSoundDeliver deliver_sounds, void *user)
{
    SimInput no_edges;
    int updates = 0;

    memset(&no_edges, 0, sizeof(no_edges));

    /* hold newly sampled edges until an update actually runs: on a fast
     * display many rendered frames run zero updates, and a press
     * sampled on such a frame must survive until the next one */
    if (input) {
        if (sched_held_valid) {
            /* two samples without an intervening update merge: the
             * edges are one update wide, so composed edges simply apply
             * together */
            sched_held.pressed_right |= input->pressed_right;
            sched_held.pressed_left |= input->pressed_left;
            sched_held.pressed_up |= input->pressed_up;
            sched_held.pressed_down |= input->pressed_down;
            sched_held.pressed_space |= input->pressed_space;
            sched_held.pressed_enter |= input->pressed_enter;
            sched_held.pressed_e |= input->pressed_e;
            sched_held.pressed_r |= input->pressed_r;
            sched_held.pressed_undo |= input->pressed_undo;
            sched_held.pressed_any |= input->pressed_any;
        } else {
            sched_held = *input;
            sched_held_valid = 1;
        }
    }

    if (dt_ms < 0.0) dt_ms = 0.0;
    sched_acc_ms += dt_ms;

    while (sched_acc_ms >= SIM_STEP_MS) {
        const SimInput *upd_input = sched_held_valid ? &sched_held : NULL;

        sim_tick(world_ptr(), upd_input);
        sched_held_valid = 0; /* edges are consumed once */
        /* the view pass consumes this update's fx events and the hook
         * delivers its sounds — both before the next update's
         * events_clear(), so nothing accumulates and nothing is
         * dropped */
        world_view_tick(upd_input ? upd_input : &no_edges);
        if (deliver_sounds) deliver_sounds(user);

        sched_acc_ms -= SIM_STEP_MS;
        updates++;
        if (updates >= SIM_MAX_CATCHUP) {
            /* massive stall: drop the backlog and resume at the next
             * update instead of replaying the whole gap */
            sched_acc_ms = 0.0;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Room management                                                     */
/* ------------------------------------------------------------------ */

void sim_room_goto(SimWorld *w, int room_index)
{
    /* the catalog owns the play bound: the two authored rooms past the
     * in-play prefix stay unreachable */
    if (room_index < 0 || room_index >= longo_room_play_count()) return;
    load_room(w, room_index);
}

void sim_room_goto_next(SimWorld *w)
{
    sim_room_goto(w, w->room_index + 1); /* out of range keeps the room */
}

void sim_room_restart(SimWorld *w) { sim_room_goto(w, w->room_index); }

/* NEW GAME: the one complete reset.  Reinitializes the SimWorld (rng
 * seed, tick = 1), zeroes the view module's animation clocks
 * (view_reset: a new game must not inherit the previous session's
 * frame time), resets the transition wipe and loads the title room
 * (which resets every room-scoped object, see load_room).  Room loads
 * and retries go through load_room and reset room-scoped state only. */
void sim_init(unsigned int seed)
{
    SimWorld *w = &game_world;
    memset(w, 0, sizeof(*w));
    w->rng = seed ? seed : 0x1234u;
    /* tick starts at 1 so room_loaded_tick comparisons (load_tick ==
     * current tick) key off a nonzero value */
    w->tick = 1;
    /* a new game checks every authored room up front, so malformed data
     * fails here rather than rooms later (load_room re-checks the room
     * it loads) */
    for (int i = 0; i < longo_room_count(); i++) validate_room_or_abort(i);
    /* The transition outlives room loads (its object is placed only in
     * the title room but must keep wiping across rooms); a new game
     * still starts from a clean wipe. */
    transition_reset();
    view_reset();
    load_room(w, SIM_ROOM_TITLE);
    /* a new game starts the fixed-step schedule over too */
    sched_acc_ms = 0.0;
    sched_held_valid = 0;
}
