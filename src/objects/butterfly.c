#include "butterfly.h"

#include <math.h>
#include <string.h>

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
static unsigned rng = 0xbee9u;

static unsigned bf_rng_next(void)
{
    unsigned int x = rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng = x ? x : 0x9e3779b9u;
    return x;
}

static float bf_random(float max)
{
    return (float)(bf_rng_next() & 0xFFFFFF) / (float)0x1000000 * max;
}

static float bf_random_range(float lo, float hi)
{
    return lo + bf_random(hi - lo);
}

static float f_clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float len_dir_x(float len, float dir)
{
    return cosf(dir * (3.14159265f / 180.0f)) * len;
}

static float len_dir_y(float len, float dir)
{
    return -sinf(dir * (3.14159265f / 180.0f)) * len;
}

static float point_direction(float x1, float y1, float x2, float y2)
{
    float dir = atan2f(-(y2 - y1), x2 - x1) * (180.0f / 3.14159265f);
    if (dir < 0.0f) dir += 360.0f;
    return dir;
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

/* oButterfly step + alarm port: drift, re-aim home, scatter on bark. */
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
    int frame = (int)view_apple_clock() % 8; /* the fly flaps with the
                                              * apple clock, as upstream */
    for (int i = 0; i < BUTTERFLY_MAX; i++) {
        if (!flies[i].alive) continue;
        if (shadow)
            view_sprite(0, i, LONGO_SPR_FLY, frame, flies[i].x,
                        flies[i].y + 16.0f, 1.0f, 0.6f, 0.0f, tint, 1.0f);
        else
            view_sprite(-500, i, LONGO_SPR_FLY, frame, flies[i].x, flies[i].y,
                        1.0f, 1.0f, 0.0f, tint, 1.0f);
    }
}
