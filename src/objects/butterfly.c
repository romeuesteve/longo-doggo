#include "butterfly.h"

#include <math.h>
#include <string.h>

#include "../core/sim_math.h"
#include "../core/rng.h"
#include "../core/view.h"
#include "dog.h"

#define BUTTERFLY_MAX 8

typedef struct Butterfly {
    bool alive;
    float x, y;
    float xstart, ystart;
    float hspd, vspd;
    float dir;
    int timer;
} Butterfly;

static Butterfly flies[BUTTERFLY_MAX];
static Rng bf_rng = { 0xbee9u };

static float bf_random(float max) { return rng_float(&bf_rng, max); }

static float bf_random_range(float lo, float hi)
{
    return rng_range(&bf_rng, lo, hi);
}

static float f_clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void butterfly_reset(void)
{
    memset(flies, 0, sizeof(flies));
}

void butterfly_place(float x, float y)
{
    for (int i = 0; i < BUTTERFLY_MAX; i++) {
        Butterfly *f = &flies[i];
        if (f->alive) continue;
        f->alive = true;
        f->x = f->xstart = x;
        f->y = f->ystart = y;
        f->hspd = f->vspd = 0.0f;
        f->dir = 0.0f;
        f->timer = 10;
        return;
    }
}

/* Drift, re-aim home, scatter on bark. */
void butterfly_tick(const SimInput *input)
{
    for (int i = 0; i < BUTTERFLY_MAX; i++) {
        Butterfly *f = &flies[i];
        if (!f->alive) continue;
        f->hspd = f_clamp(f->hspd, -0.3f, 0.3f);
        f->vspd = f_clamp(f->vspd, -0.3f, 0.3f);
        f->hspd += len_dir_x(0.001f, f->dir);
        f->vspd += len_dir_y(0.001f, f->dir);
        f->x += f->hspd;
        f->y += f->vspd;

        if (dog_alive() && input->pressed_space) {
            float dx = f->x - dog_visual_x();
            float dy = f->y - dog_visual_y();
            if (sqrtf(dx * dx + dy * dy) < 10.0f) {
                f->dir = point_direction(dog_visual_x(), dog_visual_y(),
                                         f->x + 8.0f, f->y + 8.0f);
                f->hspd = len_dir_x(0.3f, f->dir);
                f->vspd = len_dir_y(0.3f, f->dir);
            }
        }

        if (--f->timer <= 0) {
            float dx = f->x - f->xstart;
            float dy = f->y - f->ystart;
            if (sqrtf(dx * dx + dy * dy) < 6.0f) {
                f->dir = bf_random(360.0f);
            } else {
                f->dir = point_direction(f->x, f->y, f->xstart, f->ystart);
                f->hspd -= 0.003f;
                f->vspd -= 0.003f;
            }
            f->timer = (int)bf_random_range(10.0f, 60.0f);
        }
    }
}

void butterfly_draw(int shadow)
{
    view_layer(shadow ? VIEW_SHADOW : VIEW_WORLD);
    ViewColor tint = shadow ? view_rgb(0, 0, 0) : view_rgb(255, 255, 255);
    /* the fly flaps with the apple clock, as upstream */
    int frame = (int)view_sprite_clock(LONGO_SPR_APPLE) % 8;
    for (int i = 0; i < BUTTERFLY_MAX; i++) {
        if (!flies[i].alive) continue;
        if (shadow)
            view_sprite(0, LONGO_SPR_FLY, frame, flies[i].x,
                        flies[i].y + 16.0f, 1.0f, 0.6f, 0.0f, tint, 1.0f);
        else
            view_sprite(VIEW_DEPTH_BARK, LONGO_SPR_FLY, frame, flies[i].x,
                        flies[i].y, 1.0f, 1.0f, 0.0f, tint, 1.0f);
    }
}
