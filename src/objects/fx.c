#include "fx.h"

#include <math.h>
#include <string.h>

#include "../core/events.h"
#include "../core/sim_math.h"
#include "../core/rng.h"

#define FX_MAX_SMOKE 160
#define FX_MAX_POPUPS 16
#define FX_MAX_BARKS 8
#define FX_MAX_SINKS 8

typedef struct Smoke {
    bool alive;
    float x, y;
    float dir;   /* motion direction, degrees */
    float speed;
    float angle; /* sprite rotation */
    float spin;
    float scale;
} Smoke;

typedef struct Popup {
    bool alive;
    float x, y;
    float alpha;
    int variant;
} Popup;

typedef struct Bark {
    bool alive;
    float x, y;
    float angle;
    float frame;
} Bark;

typedef struct Sink {
    bool alive;
    float x, y;
    float tx, ty;
} Sink;

static Smoke smoke[FX_MAX_SMOKE];
static int smoke_cnt;
static Popup popups[FX_MAX_POPUPS];
static int popup_cnt;
static Bark barks[FX_MAX_BARKS];
static int bark_cnt;
static Sink sinks[FX_MAX_SINKS];
static int sink_cnt;

static Rng fx_rng = { 0x1234u };

static float fx_random(float max) { return rng_float(&fx_rng, max); }

static float fx_random_range(float lo, float hi)
{
    return rng_range(&fx_rng, lo, hi);
}

void fx_reset(void)
{
    memset(smoke, 0, sizeof(smoke));
    smoke_cnt = 0;
    memset(popups, 0, sizeof(popups));
    popup_cnt = 0;
    memset(barks, 0, sizeof(barks));
    bark_cnt = 0;
    memset(sinks, 0, sizeof(sinks));
    sink_cnt = 0;
}

void fx_counts(int *smoke, int *popups, int *barks, int *sinks)
{
    if (smoke) *smoke = smoke_cnt;
    if (popups) *popups = popup_cnt;
    if (barks) *barks = bark_cnt;
    if (sinks) *sinks = sink_cnt;
}

/* Spawning reuses dead slots, so the counts are high-water marks of
 * simultaneously-alive fx.  The pools used to be append-only: after a
 * few minutes of play every new poof was silently dropped (sinks died
 * first at 8, then smoke at 160 — hole fills and the house opening
 * went quiet). */
static Smoke *smoke_slot(void)
{
    for (int i = 0; i < smoke_cnt; i++)
        if (!smoke[i].alive) return &smoke[i];
    return smoke_cnt < FX_MAX_SMOKE ? &smoke[smoke_cnt++] : NULL;
}

static Popup *popup_slot(void)
{
    for (int i = 0; i < popup_cnt; i++)
        if (!popups[i].alive) return &popups[i];
    return popup_cnt < FX_MAX_POPUPS ? &popups[popup_cnt++] : NULL;
}

static Bark *bark_slot(void)
{
    for (int i = 0; i < bark_cnt; i++)
        if (!barks[i].alive) return &barks[i];
    return bark_cnt < FX_MAX_BARKS ? &barks[bark_cnt++] : NULL;
}

static Sink *sink_slot(void)
{
    for (int i = 0; i < sink_cnt; i++)
        if (!sinks[i].alive) return &sinks[i];
    return sink_cnt < FX_MAX_SINKS ? &sinks[sink_cnt++] : NULL;
}

static void spawn_smoke_puff(float x, float y)
{
    Smoke *s = smoke_slot();
    if (!s) return;
    s->alive = true;
    s->x = x;
    s->y = y;
    s->spin = fx_random_range(-20.0f, 20.0f);
    s->angle = fx_random(360.0f);
    s->dir = s->angle;
    s->speed = fx_random(1.0f);
    s->scale = 1.0f;
}

static void spawn_smoke_burst(float x, float y, int count)
{
    for (int i = 0; i < count; i++)
        spawn_smoke_puff(x + fx_random_range(-3.0f, 3.0f),
                         y + fx_random_range(-3.0f, 3.0f));
}

void fx_tick(void)
{
    /* advance existing particles (smoke motion + draw mutations) */
    for (int i = 0; i < smoke_cnt; i++) {
        Smoke *s = &smoke[i];
        if (!s->alive) continue;
        s->x += len_dir_x(s->speed, s->dir);
        s->y += len_dir_y(s->speed, s->dir);
        s->angle += s->spin;
        s->scale = f_lerp(s->scale, 0.0f, 0.04f);
        s->speed = f_lerp(s->speed, 0.0f, 0.02f);
        if (s->scale < 0.05f) s->alive = false;
    }
    for (int i = 0; i < popup_cnt; i++) {
        Popup *o = &popups[i];
        if (!o->alive) continue;
        o->y = f_lerp(o->y, o->y - 16.0f, 0.1f);
        o->alpha -= 0.02f;
        if (o->alpha <= 0.1f) o->alive = false;
    }
    for (int i = 0; i < bark_cnt; i++) {
        Bark *b = &barks[i];
        if (!b->alive) continue;
        b->frame += (float)longo_sprite_info(LONGO_SPR_BARK)->fps / 60.0f;
        if (b->frame >= (float)longo_sprite_frames(LONGO_SPR_BARK))
            b->alive = false;
    }
    for (int i = 0; i < sink_cnt; i++) {
        Sink *s = &sinks[i];
        if (!s->alive) continue;
        s->x = f_lerp(s->x, s->tx, 0.25f);
        s->y = f_lerp(s->y, s->ty, 0.25f);
        if (fabsf(s->x - s->tx) < 1.0f && fabsf(s->y - s->ty) < 1.0f) {
            s->alive = false;
            spawn_smoke_burst(s->tx + 8.0f, s->ty + 8.0f, 7);
        }
    }

    /* consume the sim's fx events */
    FxEvent ev[EVENTS_MAX_FX];
    int count = events_poll_fx(ev);
    for (int i = 0; i < count; i++) {
        switch (ev[i].kind) {
        case FX_SMOKE_BURST:
            spawn_smoke_burst(ev[i].x, ev[i].y, ev[i].count);
            break;
        case FX_BARK: {
            Bark *b = bark_slot();
            if (b) {
                b->alive = true;
                b->x = ev[i].x;
                b->y = ev[i].y;
                b->angle = ev[i].angle;
                b->frame = 0.0f;
            }
            break;
        }
        case FX_ONE: {
            Popup *o = popup_slot();
            if (o) {
                o->alive = true;
                o->x = ev[i].x;
                o->y = ev[i].y;
                o->alpha = 1.0f;
                o->variant = ev[i].variant;
            }
            break;
        }
        case FX_BOX_SINK: {
            Sink *s = sink_slot();
            if (s) {
                s->alive = true;
                s->x = ev[i].x;
                s->y = ev[i].y;
                s->tx = ev[i].tx;
                s->ty = ev[i].ty;
            }
            break;
        }
        default:
            break;
        }
    }
}

void fx_draw(void)
{
    view_layer(VIEW_WORLD);
    ViewColor cream = view_rgb(255, 235, 204);
    ViewColor gold = view_rgb(235, 176, 81);
    static const float offsets[5][2] = {
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { 0, 0 }
    };
    for (int i = 0; i < smoke_cnt; i++) {
        Smoke *s = &smoke[i];
        if (!s->alive) continue;
        for (int k = 0; k < 5; k++)
            view_sprite(VIEW_DEPTH_SMOKE, LONGO_SPR_SMOKE, 0,
                        s->x + offsets[k][0], s->y + offsets[k][1], s->scale,
                        s->scale, s->angle, k == 4 ? cream : gold, 1.0f);
    }
    for (int i = 0; i < sink_cnt; i++) {
        Sink *s = &sinks[i];
        if (!s->alive) continue;
        view_sprite(VIEW_DEPTH_BOX_BASE, LONGO_SPR_BOX, 0, s->x, s->y, 1.0f,
                    1.0f, 0.0f, view_rgb(255, 255, 255), 1.0f);
    }
    for (int i = 0; i < bark_cnt; i++) {
        Bark *b = &barks[i];
        if (!b->alive) continue;
        view_sprite(VIEW_DEPTH_BARK, LONGO_SPR_BARK, (int)b->frame, b->x,
                    b->y, 1.0f, 1.0f, b->angle, view_rgb(255, 255, 255),
                    1.0f);
    }
    for (int i = 0; i < popup_cnt; i++) {
        Popup *o = &popups[i];
        if (!o->alive) continue;
        view_sprite(VIEW_DEPTH_ONE, LONGO_SPR_ONE, o->variant, o->x, o->y,
                    1.0f, 1.0f, 0.0f, view_rgb(255, 255, 255), o->alpha);
    }
}
