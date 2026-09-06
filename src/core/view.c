#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIEW_SPRITE_CLOCKS 32

typedef struct ViewLayerData {
    ViewItem items[VIEW_MAX_ITEMS];
    int count;
    /* draw order as indices into items[]; sorting indices keeps the
     * per-frame sort from memmove-ing the ~250-byte item structs */
    int order[VIEW_MAX_ITEMS];
} ViewLayerData;

static ViewLayerData layers[3];
static ViewLayer current = VIEW_WORLD;
static double time_ms;
static float sprite_clocks[VIEW_SPRITE_CLOCKS];

void view_begin_frame(void)
{
    layers[VIEW_SHADOW].count = 0;
    layers[VIEW_WORLD].count = 0;
    layers[VIEW_GUI].count = 0;
    time_ms += 1000.0 / 60.0;
    /* advance each sprite clock by fps / 60 frames */
    for (int s = 1; s < VIEW_SPRITE_CLOCKS; s++) {
        const LongoSpriteInfo *info = longo_sprite_info(s);
        if (info != NULL && info->fps > 0)
            sprite_clocks[s] += (float)info->fps / 60.0f;
    }
}

void view_layer(ViewLayer layer) { current = layer; }

static ViewLayer sort_layer;
static const ViewItem *sort_items;

static int item_compare(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;
    const ViewItem *va = &sort_items[ia], *vb = &sort_items[ib];
    if (va->depth != vb->depth) return vb->depth - va->depth;
    /* order is the push index, so ties keep push order and qsort is
     * deterministic */
    return va->order - vb->order;
}

void view_sort(void)
{
    for (int l = 0; l < 3; l++) {
        int n = layers[l].count;
        sort_layer = (ViewLayer)l;
        sort_items = layers[l].items;
        for (int i = 0; i < n; i++) layers[l].order[i] = i;
        qsort(layers[l].order, (size_t)n, sizeof(int), item_compare);
    }
}

const ViewItem *view_items(ViewLayer layer, int *count)
{
    *count = layers[layer].count;
    return layers[layer].items;
}

int view_order_at(ViewLayer layer, int i)
{
    return layers[layer].order[i];
}

float view_sprite_clock(int sprite)
{
    if (sprite < 0 || sprite >= VIEW_SPRITE_CLOCKS) return 0.0f;
    return sprite_clocks[sprite];
}

double view_time_ms(void) { return time_ms; }

ViewColor view_rgb(int r, int g, int b)
{
    ViewColor c = { (uint8_t)r, (uint8_t)g, (uint8_t)b, 255 };
    return c;
}

static ViewItem *push(void)
{
    ViewLayerData *l = &layers[current];
    ViewItem *it;
    if (l->count >= VIEW_MAX_ITEMS) return NULL;
    it = &l->items[l->count];
    it->order = l->count; /* push order is the depth tie-break */
    l->count++;
    return it;
}

void view_sprite(int depth, LongoSprite sprite, int frame, float x, float y,
                 float xscale, float yscale, float rotation, ViewColor tint,
                 float alpha)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_SPRITE;
    it->depth = depth;
    it->sprite = sprite;
    it->frame = frame;
    it->x = x;
    it->y = y;
    it->xscale = xscale;
    it->yscale = yscale;
    it->rotation = rotation;
    it->color = tint;
    it->alpha = alpha;
}

void view_sprite_part(int depth, LongoSprite sprite, int frame, int src_x,
                      int src_y, int src_w, int src_h, float x, float y,
                      float xscale, float yscale, ViewColor tint,
                      float alpha)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_SPRITE_PART;
    it->depth = depth;
    it->sprite = sprite;
    it->frame = frame;
    it->x = x;
    it->y = y;
    it->w = (float)src_w;
    it->h = (float)src_h;
    it->src_x = (float)src_x;
    it->src_y = (float)src_y;
    it->xscale = xscale;
    it->yscale = yscale;
    it->color = tint;
    it->alpha = alpha;
}

void view_nine_patch(int depth, LongoSprite sprite, int frame, float x,
                     float y, float w, float h, ViewColor tint, float alpha)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_NINE_PATCH;
    it->depth = depth;
    it->sprite = sprite;
    it->frame = frame;
    it->x = x;
    it->y = y;
    it->w = w;
    it->h = h;
    it->color = tint;
    it->alpha = alpha;
}

void view_line(int depth, float x1, float y1, float x2, float y2, float width,
               ViewColor c1, ViewColor c2)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_LINE;
    it->depth = depth;
    it->x = x1;
    it->y = y1;
    it->x2 = x2;
    it->y2 = y2;
    it->radius = width;
    it->color = c1;
    it->color2 = c2;
}

void view_circle(int depth, float x, float y, float radius, ViewColor c1,
                 ViewColor c2)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_CIRCLE;
    it->depth = depth;
    it->x = x;
    it->y = y;
    it->radius = radius;
    it->color = c1;
    it->color2 = c2;
}

void view_rect(int depth, float x, float y, float w, float h, ViewColor color)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_RECT;
    it->depth = depth;
    it->x = x;
    it->y = y;
    it->w = w;
    it->h = h;
    it->color = color;
}

void view_text(int depth, int font_id, const char *text, float x, float y,
               float scale, ViewColor color)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_TEXT;
    it->depth = depth;
    it->font_id = font_id;
    snprintf(it->text, sizeof(it->text), "%s", text ? text : "");
    it->x = x;
    it->y = y;
    it->xscale = scale;
    it->color = color;
}

void view_text_wrapped(int depth, int font_id, const char *text, float x,
                       float y, float line_sep, float width, float scale,
                       ViewColor color)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_TEXT_WRAPPED;
    it->depth = depth;
    it->font_id = font_id;
    snprintf(it->text, sizeof(it->text), "%s", text ? text : "");
    it->x = x;
    it->y = y;
    it->line_sep = line_sep;
    it->text_width = width;
    it->xscale = scale;
    it->color = color;
}

void view_shadow_composite(int depth)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_SHADOW_COMPOSITE;
    it->depth = depth;
}

void view_tile_layers(int depth, int index)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_TILE_LAYERS;
    it->depth = depth;
    it->frame = index;
}
