#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ViewLayerData {
    ViewItem items[VIEW_MAX_ITEMS];
    int count;
} ViewLayerData;

static ViewLayerData layers[3];
static ViewLayer current = VIEW_WORLD;
static double time_ms;
static float flower_clock, apple_clock, pear_clock, fly_clock, button_clock;

void view_begin_frame(void)
{
    layers[VIEW_SHADOW].count = 0;
    layers[VIEW_WORLD].count = 0;
    layers[VIEW_GUI].count = 0;
    time_ms += 1000.0 / 60.0;
    /* image_index += image_speed * fps / 60, like GameMaker */
    flower_clock += (float)longo_sprite_info(LONGO_SPR_FLOWER)->fps / 60.0f;
    apple_clock += (float)longo_sprite_info(LONGO_SPR_APPLE)->fps / 60.0f;
    pear_clock += (float)longo_sprite_info(LONGO_SPR_PEAR)->fps / 60.0f;
    fly_clock += (float)longo_sprite_info(LONGO_SPR_FLY)->fps / 60.0f;
    button_clock += (float)longo_sprite_info(LONGO_SPR_BUTTON)->fps / 60.0f;
}

void view_layer(ViewLayer layer) { current = layer; }

static int item_compare(const void *a, const void *b)
{
    const ViewItem *ia = a, *ib = b;
    if (ia->depth != ib->depth) return ib->depth - ia->depth;
    return ia->order - ib->order;
}

void view_sort(void)
{
    for (int l = 0; l < 3; l++)
        qsort(layers[l].items, (size_t)layers[l].count, sizeof(ViewItem),
              item_compare);
}

const ViewItem *view_items(ViewLayer layer, int *count)
{
    *count = layers[layer].count;
    return layers[layer].items;
}

float view_flower_clock(void) { return flower_clock; }
float view_apple_clock(void) { return apple_clock; }
float view_pear_clock(void) { return pear_clock; }
float view_fly_clock(void) { return fly_clock; }
float view_button_clock(void) { return button_clock; }
double view_time_ms(void) { return time_ms; }

ViewColor view_rgb(int r, int g, int b)
{
    ViewColor c = { (uint8_t)r, (uint8_t)g, (uint8_t)b, 255 };
    return c;
}

static ViewItem *push(void)
{
    ViewLayerData *l = &layers[current];
    if (l->count >= VIEW_MAX_ITEMS) return NULL;
    return &l->items[l->count++];
}

void view_sprite(int depth, int order, LongoSprite sprite, int frame, float x,
                 float y, float xscale, float yscale, float rotation,
                 ViewColor tint, float alpha)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_SPRITE;
    it->depth = depth;
    it->order = order;
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

void view_sprite_part(int depth, int order, LongoSprite sprite, int frame,
                      int src_x, int src_y, int src_w, int src_h, float x,
                      float y, float xscale, float yscale, ViewColor tint,
                      float alpha)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_SPRITE_PART;
    it->depth = depth;
    it->order = order;
    it->sprite = sprite;
    it->frame = frame;
    it->x = x;
    it->y = y;
    it->w = (float)src_w;
    it->h = (float)src_h;
    it->radius = (float)src_x; /* stash source origin in spare fields */
    it->xscale = xscale;
    it->yscale = yscale;
    it->rotation = (float)src_y; /* stash */
    it->color = tint;
    it->alpha = alpha;
}

void view_line(int depth, int order, float x1, float y1, float x2, float y2,
               float width, ViewColor c1, ViewColor c2)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_LINE;
    it->depth = depth;
    it->order = order;
    it->x = x1;
    it->y = y1;
    it->x2 = x2;
    it->y2 = y2;
    it->radius = width;
    it->color = c1;
    it->color2 = c2;
}

void view_circle(int depth, int order, float x, float y, float radius,
                 ViewColor c1, ViewColor c2)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_CIRCLE;
    it->depth = depth;
    it->order = order;
    it->x = x;
    it->y = y;
    it->radius = radius;
    it->color = c1;
    it->color2 = c2;
}

void view_rect(int depth, int order, float x, float y, float w, float h,
               ViewColor color)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_RECT;
    it->depth = depth;
    it->order = order;
    it->x = x;
    it->y = y;
    it->w = w;
    it->h = h;
    it->color = color;
}

void view_text(int depth, int order, int font_id, const char *text, float x,
               float y, float scale, ViewColor color)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_TEXT;
    it->depth = depth;
    it->order = order;
    it->font_id = font_id;
    snprintf(it->text, sizeof(it->text), "%s", text ? text : "");
    it->x = x;
    it->y = y;
    it->xscale = scale;
    it->color = color;
}

void view_text_wrapped(int depth, int order, int font_id, const char *text,
                       float x, float y, float line_sep, float width,
                       float scale, ViewColor color)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_TEXT_WRAPPED;
    it->depth = depth;
    it->order = order;
    it->font_id = font_id;
    snprintf(it->text, sizeof(it->text), "%s", text ? text : "");
    it->x = x;
    it->y = y;
    it->line_sep = line_sep;
    it->text_width = width;
    it->xscale = scale;
    it->color = color;
}

void view_shadow_composite(int depth, int order)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_SHADOW_COMPOSITE;
    it->depth = depth;
    it->order = order;
}

void view_tile_layers(int depth, int order, int index)
{
    ViewItem *it = push();
    if (!it) return;
    it->kind = VIEW_ITEM_TILE_LAYERS;
    it->depth = depth;
    it->order = order;
    it->frame = index;
}
