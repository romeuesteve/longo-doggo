/*
 * View kernel: the shared drawing vocabulary between the object scripts
 * and the render backend.
 *
 * Object scripts push draw items (sprites, lines, circles, text, rects)
 * tagged with a depth into one of three layers (shadow surface, world,
 * GUI); the backend sorts each layer by depth — push order breaks ties —
 * and replays the items with raylib.  The depths are the recovered
 * GameMaker instance depths; see VIEW_DEPTH_* below.  The kernel also
 * owns the shared animation clocks and the frame time (all easing runs
 * at a fixed 60 Hz, matching the original tick-coupled lerp constants).
 */
#ifndef LONGO_VIEW_H
#define LONGO_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#include "../sprites.h"

/* Worst-case push load: 160 smoke puffs x 5 sprites alone exceed the old
 * 512, and overflow dropped draws silently. */
#define VIEW_MAX_ITEMS 1024

typedef enum ViewLayer {
    VIEW_SHADOW = 0, /* global.shadow_surf, composited at 0.2 alpha */
    VIEW_WORLD,      /* application surface (depth-sorted) */
    VIEW_GUI         /* GUI surface: bloom composite + dialogue/wipes */
} ViewLayer;

/* The recovered GameMaker instance depths (draw order; larger depth
 * draws earlier, i.e. further back).  The box's depth is dynamic:
 * VIEW_DEPTH_BOX_BASE - y / 6. */
enum {
    VIEW_DEPTH_ONE = -200000,    /* apple/pear popups (oOne) */
    VIEW_DEPTH_SMOKE = -1000,    /* smoke puffs, 5 sprites each */
    VIEW_DEPTH_TITLE = -600,     /* title logo + prompt */
    VIEW_DEPTH_BARK = -500,      /* bark sprites and oButterfly */
    VIEW_DEPTH_GOAL_OVERLAY = -220, /* oGoalUp squash redraw of the top */
    VIEW_DEPTH_GOAL = -180,      /* oGoal house base sprite */
    VIEW_DEPTH_BOX_BASE = -100,  /* boxes and the box-sink fx */
    VIEW_DEPTH_DOG = 0,          /* the dog (and everything shadowed) */
    VIEW_DEPTH_ITEM = 100,       /* apples/pears and oDoor */
    VIEW_DEPTH_GROUND = 200,     /* buttons, holes, flowers */
    VIEW_DEPTH_TILES = 700       /* background + Tiles_3 tile layers */
};

typedef struct ViewColor {
    uint8_t r, g, b, a;
} ViewColor;

typedef enum ViewItemKind {
    VIEW_ITEM_SPRITE,
    VIEW_ITEM_SPRITE_PART,
    VIEW_ITEM_NINE_PATCH, /* corners native, sides + centre stretched */
    VIEW_ITEM_LINE,
    VIEW_ITEM_CIRCLE,
    VIEW_ITEM_RECT,
    VIEW_ITEM_TEXT,
    VIEW_ITEM_TEXT_WRAPPED,
    VIEW_ITEM_SHADOW_COMPOSITE,
    VIEW_ITEM_TILE_LAYERS /* background (0) or the Tiles_3 layer (1) */
} ViewItemKind;

typedef struct ViewItem {
    ViewItemKind kind;
    int depth;
    int order; /* push index within the layer; the sort's tie-break */
    LongoSprite sprite;
    int frame;
    float x, y;   /* position / line start / rect origin */
    float x2, y2; /* line end */
    float w, h;   /* sprite-part source size / nine-patch size / rect size */
    float src_x, src_y; /* sprite-part source origin */
    float xscale, yscale;
    float rotation; /* GameMaker degrees: positive = counterclockwise on
                     * screen; the render backend converts to its API */
    float alpha;
    float radius;     /* circle radius / line width */
    ViewColor color;  /* primary (line gradient start, circle top) */
    ViewColor color2; /* secondary (line gradient end, circle bottom) */
    char text[192];   /* text items; the credits line needs ~170 chars */
    int font_id;      /* recovered font asset (0 bold = left-aligned,
                       * 1 regular / 2 digits = centered) */
    float text_width; /* wrap width */
    float line_sep;   /* line separation for wrapped text */
} ViewItem;

void view_begin_frame(void);      /* clears all layers, advances time */
void view_layer(ViewLayer layer); /* selects the push target */
void view_sort(void);             /* depth-sort every layer */

const ViewItem *view_items(ViewLayer layer, int *count);

/* Animation clocks (image_index += fps / 60 per frame), one per sprite.
 * Consumers share a sprite's clock where the original did: the fly flaps
 * on the apple clock, the dog's face/tail on the flower clock. */
float view_sprite_clock(int sprite);
double view_time_ms(void);

/* Draw pushers (into the current layer). */
void view_sprite(int depth, LongoSprite sprite, int frame, float x, float y,
                 float xscale, float yscale, float rotation, ViewColor tint,
                 float alpha);
void view_sprite_part(int depth, LongoSprite sprite, int frame, int src_x,
                      int src_y, int src_w, int src_h, float x, float y,
                      float xscale, float yscale, ViewColor tint,
                      float alpha);
/* (x, y) is the panel's top-left; (w, h) its size in logical pixels.  The
 * sprite's corners keep their native size, edges and centre stretch. */
void view_nine_patch(int depth, LongoSprite sprite, int frame, float x,
                     float y, float w, float h, ViewColor tint, float alpha);
void view_line(int depth, float x1, float y1, float x2, float y2, float width,
               ViewColor c1, ViewColor c2);
void view_circle(int depth, float x, float y, float radius, ViewColor c1,
                 ViewColor c2);
void view_rect(int depth, float x, float y, float w, float h, ViewColor color);
void view_text(int depth, int font_id, const char *text, float x, float y,
               float scale, ViewColor color);
void view_text_wrapped(int depth, int font_id, const char *text, float x,
                       float y, float line_sep, float width, float scale,
                       ViewColor color);
void view_shadow_composite(int depth);
void view_tile_layers(int depth, int index);

ViewColor view_rgb(int r, int g, int b);

#endif /* LONGO_VIEW_H */
