#include "pres.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

static float f_lerp(float a, float b, float t) { return a + (b - a) * t; }
static float f_clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static float len_dir_x(float len, float dir)
{
    return cosf(dir * (M_PI / 180.0f)) * len;
}
static float len_dir_y(float len, float dir)
{
    return -sinf(dir * (M_PI / 180.0f)) * len;
}
static float point_direction(float x1, float y1, float x2, float y2)
{
    float dir = atan2f(-(y2 - y1), x2 - x1) * (180.0f / M_PI);
    if (dir < 0.0f) dir += 360.0f;
    return dir;
}

static unsigned pres_rng_next(Pres *p)
{
    unsigned int x = p->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    p->rng = x ? x : 0x9e3779b9u;
    return p->rng;
}

static float pres_random(Pres *p, float max)
{
    return (float)(pres_rng_next(p) & 0xFFFFFF) / (float)0x1000000 * max;
}

static float pres_random_range(Pres *p, float lo, float hi)
{
    return lo + pres_random(p, hi - lo);
}

/* ------------------------------------------------------------------ */
/* FX spawners                                                         */
/* ------------------------------------------------------------------ */

static PresSmoke *spawn_smoke(Pres *p)
{
    if (p->smoke_count >= PRES_MAX_SMOKE) return NULL;
    return &p->smoke[p->smoke_count++];
}

/* oSmoke create + built-in motion + draw-mutation port, one puff. */
static void spawn_smoke_puff(Pres *p, float x, float y)
{
    PresSmoke *s = spawn_smoke(p);
    if (!s) return;
    s->alive = 1;
    s->x = x;
    s->y = y;
    s->spin = pres_random_range(p, -20.0f, 20.0f);
    s->angle = pres_random(p, 360.0f);
    s->dir = s->angle;
    s->speed = pres_random(p, 1.0f);
    s->scale = 1.0f;
}

static void spawn_smoke_burst(Pres *p, float x, float y, int count)
{
    for (int i = 0; i < count; i++)
        spawn_smoke_puff(p, x + pres_random_range(p, -3.0f, 3.0f),
                         y + pres_random_range(p, -3.0f, 3.0f));
}

static void spawn_popup(Pres *p, float x, float y, int variant)
{
    if (p->popup_count >= PRES_MAX_POPUPS) return;
    PresPopup *o = &p->popups[p->popup_count++];
    o->alive = 1;
    o->x = x;
    o->y = y;
    o->alpha = 1.0f;
    o->variant = variant;
}

static void spawn_bark(Pres *p, float x, float y, float angle)
{
    if (p->bark_count >= PRES_MAX_BARKS) return;
    PresBark *b = &p->barks[p->bark_count++];
    b->alive = 1;
    b->x = x;
    b->y = y;
    b->angle = angle;
    b->frame = 0.0f;
}

static void spawn_sink(Pres *p, float x, float y, float tx, float ty)
{
    if (p->sink_count >= PRES_MAX_SINKS) return;
    PresSink *s = &p->sinks[p->sink_count++];
    s->alive = 1;
    s->x = x;
    s->y = y;
    s->tx = tx;
    s->ty = ty;
}

/* ------------------------------------------------------------------ */
/* Room decor scan + state snap                                        */
/* ------------------------------------------------------------------ */

static void snap_dog(Pres *p, const SimWorld *w)
{
    p->dog_x = (float)(w->dog.cx * SIM_CELL + 8);
    p->dog_y = (float)(w->dog.cy * SIM_CELL + 8);
    for (int i = 0; i < w->dog.length; i++) {
        uint16_t cell = w->dog.chain[i];
        p->part_cell[i] = cell;
        p->part_x[i] = (float)(sim_cell_x(cell) * SIM_CELL + 8);
        p->part_y[i] = (float)(sim_cell_y(cell) * SIM_CELL + 8);
        p->part_wiggle[i] = 0;
    }
    for (int i = 0; i < w->box_count; i++) {
        p->box_x[i] = (float)(sim_cell_x(w->boxes[i].cell) * SIM_CELL);
        p->box_y[i] = (float)(sim_cell_y(w->boxes[i].cell) * SIM_CELL);
    }
}

static void scan_room(Pres *p, const SimWorld *w)
{
    const LongoRoom *room = w->room;
    memset(p->flies, 0, sizeof(p->flies));
    p->flower_count = 0;
    p->shadows_present = 0;
    p->has_title_decor = 0;
    p->smoke_count = 0;
    p->popup_count = 0;
    p->bark_count = 0;
    p->sink_count = 0;

    for (int i = 0; i < room->instance_count; i++) {
        const LongoRoomInstance *inst = &room->instances[i];
        switch (inst->object) {
        case LONGO_OBJ_FLOWER:
            if (p->flower_count < PRES_MAX_FLOWERS) {
                p->flowers[p->flower_count].x = inst->x;
                p->flowers[p->flower_count].y = inst->y;
                p->flower_count++;
            }
            break;
        case LONGO_OBJ_BUTTERFLY:
            for (int f = 0; f < PRES_MAX_FLIES; f++) {
                PresButterfly *fly = &p->flies[f];
                if (!fly->alive) {
                    fly->alive = 1;
                    fly->x = fly->xstart = inst->x;
                    fly->y = fly->ystart = inst->y;
                    fly->hspd = 0.0f;
                    fly->vspd = 0.0f;
                    fly->dir = 0.0f;
                    fly->timer = 10;
                    break;
                }
            }
            break;
        case LONGO_OBJ_SHADOWS:
            p->shadows_present = 1;
            break;
        case LONGO_OBJ_TITLE:
            /* oTitle create: x = 304/4 - 20, y = 208/4 - 20 */
            p->has_title_decor = 1;
            p->title_x = 304.0f / 4.0f - 20.0f;
            p->title_y = 208.0f / 4.0f - 20.0f;
            break;
        default:
            break;
        }
    }
    p->room = room;
}

void pres_init(Pres *p, unsigned int seed)
{
    memset(p, 0, sizeof(*p));
    p->rng = seed ? seed : 0x1234u;
    p->dlg_scale_x = p->dlg_scale_y = 1.0f;
    p->goal_scale_x = p->goal_scale_y = 1.0f;
}

/* ------------------------------------------------------------------ */
/* Per-frame update                                                    */
/* ------------------------------------------------------------------ */

static void update_smoke(Pres *p)
{
    for (int i = 0; i < p->smoke_count; i++) {
        PresSmoke *s = &p->smoke[i];
        if (!s->alive) continue;
        /* built-in motion: x += lengthdir(speed, dir) */
        s->x += len_dir_x(s->speed, s->dir);
        s->y += len_dir_y(s->speed, s->dir);
        /* draw-event mutations */
        s->angle += s->spin;
        s->scale = f_lerp(s->scale, 0.0f, 0.04f);
        s->speed = f_lerp(s->speed, 0.0f, 0.02f);
        if (s->scale < 0.05f) s->alive = 0;
    }
}

static void update_popups(Pres *p)
{
    for (int i = 0; i < p->popup_count; i++) {
        PresPopup *o = &p->popups[i];
        if (!o->alive) continue;
        o->y = f_lerp(o->y, o->y - 16.0f, 0.1f);
        o->alpha -= 0.02f;
        if (o->alpha <= 0.1f) o->alive = 0;
    }
}

static void update_barks(Pres *p)
{
    for (int i = 0; i < p->bark_count; i++) {
        PresBark *b = &p->barks[i];
        if (!b->alive) continue;
        b->frame += (float)longo_sprite_info(LONGO_SPR_BARK)->fps / 60.0f;
        if (b->frame >= (float)longo_sprite_frames(LONGO_SPR_BARK))
            b->alive = 0;
    }
}

static void update_sinks(Pres *p, SimWorld *w)
{
    (void)w;
    for (int i = 0; i < p->sink_count; i++) {
        PresSink *s = &p->sinks[i];
        if (!s->alive) continue;
        /* the box lerp eased it into the hole; poof on arrival */
        s->x = f_lerp(s->x, s->tx, 0.25f);
        s->y = f_lerp(s->y, s->ty, 0.25f);
        if (fabsf(s->x - s->tx) < 1.0f && fabsf(s->y - s->ty) < 1.0f) {
            s->alive = 0;
            spawn_smoke_burst(p, s->tx + 8.0f, s->ty + 8.0f, 7);
        }
    }
}

/* Ambient butterfly port (oButterfly create/step/alarm). */
static void update_butterflies(Pres *p, const SimWorld *w,
                               const SimInput *input)
{
    for (int i = 0; i < PRES_MAX_FLIES; i++) {
        PresButterfly *fly = &p->flies[i];
        if (!fly->alive) continue;
        fly->hspd = f_clamp(fly->hspd, -0.3f, 0.3f);
        fly->vspd = f_clamp(fly->vspd, -0.3f, 0.3f);
        fly->hspd += len_dir_x(0.001f, fly->dir);
        fly->vspd += len_dir_y(0.001f, fly->dir);
        fly->x += fly->hspd;
        fly->y += fly->vspd;

        if (w->dog.alive && input->pressed_space) {
            float dx = fly->x - p->dog_x;
            float dy = fly->y - p->dog_y;
            if (sqrtf(dx * dx + dy * dy) < 10.0f) {
                fly->dir = point_direction(p->dog_x, p->dog_y,
                                           fly->x + 8.0f, fly->y + 8.0f);
                fly->hspd = len_dir_x(0.3f, fly->dir);
                fly->vspd = len_dir_y(0.3f, fly->dir);
            }
        }

        if (--fly->timer <= 0) {
            float dx = fly->x - fly->xstart;
            float dy = fly->y - fly->ystart;
            if (sqrtf(dx * dx + dy * dy) < 6.0f) {
                fly->dir = pres_random(p, 360.0f);
            } else {
                fly->dir = point_direction(fly->x, fly->y, fly->xstart,
                                           fly->ystart);
                fly->hspd -= 0.003f;
                fly->vspd -= 0.003f;
            }
            fly->timer = (int)pres_random_range(p, 10.0f, 60.0f);
        }
    }
}

static void update_goal_pulse(Pres *p, const SimWorld *w)
{
    /* oGoalUp draw-event port: the house squash-bounces through count (on
     * win) and count2 (when more length still has to be lost). */
    if (!w->goal.alive) return;
    int win_ready = w->goal.remain <= 0;
    /* oGoal instance position is the cell centre column, one cell below
     * the cell top: smoke spawns near (goal->x, goal->y - 8). */
    float px = (float)(sim_cell_x(w->goal.cell) * SIM_CELL + 8);
    float py = (float)(sim_cell_y(w->goal.cell) * SIM_CELL + 8);

    if (!win_ready) {
        if (p->goal_count2 > 0) {
            if (p->goal_count2 > 160)
                spawn_smoke_puff(p, px + pres_random_range(p, -4.0f, 4.0f),
                                 py - 8.0f + pres_random_range(p, -4.0f, 4.0f));
            p->goal_count2 -= 4;
            p->goal_scale_x =
                1.0f + longo_wave(-p->goal_count2 / 1000.0f,
                                 p->goal_count2 / 1000.0f, 0.35f, 0,
                                 p->time_ms);
            p->goal_scale_y =
                1.0f - longo_wave(-p->goal_count2 / 1000.0f,
                                 p->goal_count2 / 1000.0f, 0.35f, 0,
                                 p->time_ms);
        }
    } else {
        if (p->goal_count > 0) {
            if (p->goal_count > 160)
                spawn_smoke_puff(p, px + pres_random_range(p, -4.0f, 4.0f),
                                 py - 8.0f + pres_random_range(p, -4.0f, 4.0f));
            p->goal_count -= 4;
            p->goal_scale_x = 1.0f + longo_wave(-p->goal_count / 1000.0f,
                                               p->goal_count / 1000.0f, 0.35f,
                                               0, p->time_ms);
            p->goal_scale_y = 1.0f - longo_wave(-p->goal_count / 1000.0f,
                                               p->goal_count / 1000.0f, 0.35f,
                                               0, p->time_ms);
        }
        p->goal_count2 = 200;
    }
}

void pres_update(Pres *p, SimWorld *w, const SimInput *input)
{
    int room_changed = (p->room != w->room);
    if (room_changed) {
        scan_room(p, w);
        snap_dog(p, w);
        p->goal_count = 200;
        p->goal_count2 = 0;
        p->goal_scale_x = p->goal_scale_y = 1.0f;
        p->dlg_scale_x = p->dlg_scale_y = 1.0f;
    }
    p->time_ms += 1000.0f / 60.0f;

    /* animation clocks */
    p->flower_clock += (float)longo_sprite_info(LONGO_SPR_FLOWER)->fps / 60.0f;
    p->apple_clock += (float)longo_sprite_info(LONGO_SPR_APPLE)->fps / 60.0f;
    p->pear_clock += (float)longo_sprite_info(LONGO_SPR_PEAR)->fps / 60.0f;
    p->fly_clock += (float)longo_sprite_info(LONGO_SPR_FLY)->fps / 60.0f;
    p->button_clock += (float)longo_sprite_info(LONGO_SPR_BUTTON)->fps / 60.0f;
    p->block_clock += (float)longo_sprite_info(LONGO_SPR_BLOCK)->fps / 60.0f;

    /* dog ease: the xx/yy lerp, now the only interpolation path.
     * Targets are sprite centres (cell top-left + 8), like the original
     * instance positions. */
    float dog_tx = (float)(w->dog.cx * SIM_CELL + 8);
    float dog_ty = (float)(w->dog.cy * SIM_CELL + 8);
    if (w->dog.alive) {
        p->dog_x = f_lerp(p->dog_x, dog_tx, 0.2f);
        p->dog_y = f_lerp(p->dog_y, dog_ty, 0.2f);
    }
    for (int i = 0; i < w->dog.length; i++) {
        uint16_t cell = w->dog.chain[i];
        float tx = (float)(sim_cell_x(cell) * SIM_CELL + 8);
        float ty = (float)(sim_cell_y(cell) * SIM_CELL + 8);
        if (p->part_wiggle[i] > 0) p->part_wiggle[i]--;
        if (p->part_cell[i] != cell) {
            p->part_cell[i] = cell;
            p->part_wiggle[i] = 15; /* legs_angle = 30, alarm0 = 15 */
        }
        p->part_x[i] = f_lerp(p->part_x[i], tx, 0.2f);
        p->part_y[i] = f_lerp(p->part_y[i], ty, 0.2f);
    }

    /* box ease (0.25 like the original box lerp) */
    for (int i = 0; i < w->box_count; i++) {
        if (!w->boxes[i].alive) continue;
        float tx = (float)(sim_cell_x(w->boxes[i].cell) * SIM_CELL);
        float ty = (float)(sim_cell_y(w->boxes[i].cell) * SIM_CELL);
        p->box_x[i] = f_lerp(p->box_x[i], tx, 0.25f);
        p->box_y[i] = f_lerp(p->box_y[i], ty, 0.25f);
    }

    /* doors: open squash port (the sim destroys the door after 14 ticks) */
    for (int i = 0; i < w->door_count; i++) {
        const SimDoor *d = &w->doors[i];
        PresDoor *pd = &p->doors[i];
        if (!d->alive) continue;
        if (!d->open) {
            pd->scale_x = pd->scale_y = 1.0f;
            pd->x = (float)(sim_cell_x(d->cell) * SIM_CELL);
            pd->y = (float)(sim_cell_y(d->cell) * SIM_CELL);
            continue;
        }
        pd->scale_x = f_lerp(pd->scale_x, 1.2f, 0.1f);
        pd->scale_y = f_lerp(pd->scale_y, 0.8f, 0.1f);
        float sprite_width = 16.0f * pd->scale_x;
        float sprite_height = 16.0f * pd->scale_y;
        pd->x = (float)(sim_cell_x(d->cell) * SIM_CELL) -
                (sprite_width * (pd->scale_x - 1.0f)) / 2.0f;
        pd->y = (float)(sim_cell_y(d->cell) * SIM_CELL) -
                sprite_height * (pd->scale_y - 1.0f);
    }

    /* dialogue box scale (pop-in, bounce on advance, shrink on release) */
    if (w->dialogue.active) {
        int advance =
            (input->pressed_space || input->pressed_enter || input->pressed_e) &&
            !w->trans.close_transition;
        float target = w->dialogue.release_ticks >= 0 ? 0.6f
                                                      : w->dialogue.base_scale;
        if (advance && w->dialogue.release_ticks < 0) {
            p->dlg_scale_x = p->dlg_scale_y = 0.5f; /* press bounce */
        }
        p->dlg_scale_x = f_lerp(p->dlg_scale_x, target, 0.15f);
        p->dlg_scale_y = f_lerp(p->dlg_scale_y, target, 0.15f);
    }

    update_goal_pulse(p, w);
    update_smoke(p);
    update_popups(p);
    update_barks(p);
    update_sinks(p, w);
    update_butterflies(p, w, input);

    /* consume the sim's fx events */
    SimFx fx[SIM_MAX_FX];
    int count = sim_poll_fx(w, fx);
    for (int i = 0; i < count; i++) {
        switch (fx[i].kind) {
        case SIM_FX_SMOKE_BURST:
            spawn_smoke_burst(p, fx[i].x, fx[i].y, fx[i].count);
            break;
        case SIM_FX_BARK:
            spawn_bark(p, fx[i].x, fx[i].y, fx[i].angle);
            break;
        case SIM_FX_ONE:
            spawn_popup(p, fx[i].x, fx[i].y, fx[i].variant);
            break;
        case SIM_FX_BOX_SINK:
            spawn_sink(p, fx[i].x, fx[i].y, fx[i].tx, fx[i].ty);
            break;
        default:
            break;
        }
    }
}

float pres_part_legs_angle(const Pres *p, int part)
{
    return p->part_wiggle[part] > 0 ? 30.0f : 0.0f;
}
