#include "flower.h"

#include <string.h>

#include "../core/view.h"

#define FLOWER_MAX 64

typedef struct Flower {
    float x, y;
} Flower;

static Flower flowers[FLOWER_MAX];
static int flower_cnt;

void flower_reset(void) { flower_cnt = 0; }

void flower_place(float x, float y)
{
    if (flower_cnt >= FLOWER_MAX) return;
    flowers[flower_cnt].x = x;
    flowers[flower_cnt].y = y;
    flower_cnt++;
}

void flower_draw(int shadow)
{
    (void)shadow; /* flowers cast no shadow upstream */
    if (shadow) return;
    view_layer(VIEW_WORLD);
    ViewColor white = view_rgb(255, 255, 255);
    int frame = (int)view_flower_clock() % 4;
    for (int i = 0; i < flower_cnt; i++)
        view_sprite(200, i, LONGO_SPR_FLOWER, frame, flowers[i].x,
                    flowers[i].y, 1.0f, 1.0f, 0.0f, white, 1.0f);
}
