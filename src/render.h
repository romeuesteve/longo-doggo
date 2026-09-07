#ifndef LONGO_RENDER_H
#define LONGO_RENDER_H

#include <stdbool.h>

#include "raylib.h"
#include "core/world.h"
#include "core/view.h"
#include "room_tiles.h"

#define LONGO_LOGICAL_WIDTH 304
#define LONGO_LOGICAL_HEIGHT 208
#define LONGO_WINDOW_SCALE 4

/* The pixelated digits font, used for the in-world house counter so the
 * number stays pixelated with the rest of the game.  It only ever draws
 * a plain "%d", so just the ten digit cells of the FontDigits atlas are
 * kept; the advance is a fixed 6px. */
typedef struct LongoDigitsFont {
    Texture2D texture;
    struct {
        Rectangle source;
        unsigned char offset;
    } glyph[10]; /* cells for '0'..'9' */
    bool loaded;
} LongoDigitsFont;

/* Wrapped-text line cache size: the credits page is the longest wrap. */
#define LONGO_WRAP_CACHE_LINES 32

/* The raylib backend.  Resource lifecycle: longo_render_init() acquires
 * every resource the module uses — the render targets, sprite strips,
 * tile atlas, fonts and sounds — and the struct owns them for its whole
 * lifetime; longo_render_shutdown() releases exactly what init acquired
 * and leaves the struct zeroed, so an init/shutdown/init sequence starts
 * as clean as the first boot.  No file-scope static holds a resource. */
typedef struct LongoRender {
    /* both roots resolved once by init (see select_asset_root() and
     * resolve_ui_font_path() there); draw-time code only reads them */
    char asset_root[512];
    char ui_font_path[512];

    /* the game world, drawn at the 304x208 logical resolution */
    RenderTexture2D app_surface;
    /* GUI item canvas (transparent background): refilled per run of the
     * present pass's segment walk, so GUI items composite over the world
     * and over lower window-scale text exactly where the stream's depth
     * order puts them (the level wipe also covers world-layer text) */
    RenderTexture2D gui_surface;
    /* per-frame shadow silhouette, composited at low alpha */
    RenderTexture2D shadow_surface;

    /* The sprTile ground and the room's Tiles_3 decoration layer never
     * change within a room, so they are stamped into cached surfaces on
     * room change and composited as one quad per layer instead of a few
     * hundred per-frame tile quads. */
    struct {
        RenderTexture2D ground;
        bool ground_valid;
        RenderTexture2D decor;
        const LongoRoomTileMap *decor_for;
    } tile_cache;

    Texture2D sprites[32];
    bool sprite_loaded[32];

    Texture2D tileset1; /* backgrounds/TileSet1.png (the Tiles_3 layer) */
    bool tileset1_loaded;

    LongoDigitsFont font_digits; /* pixel digits (house counter) */

    /* UI text (Renogare), loaded on first use from ui_font_path.
     * ui_font_owned marks a LoadFontEx handle that shutdown must
     * unload; the GetFontDefault fallback is not ours to free. */
    Font ui_font;
    bool ui_font_ready;
    bool ui_font_owned;

    /* Measured line segments of the wrapped dialogue text: the wrap is
     * O(words^2) MeasureTextEx probes per frame otherwise.  Keyed by
     * text + layout; invalidated whenever any of them change. */
    struct LongoWrapCache {
        char text[192];
        Font font;
        float width, size;
        bool valid;
        int count;
        int starts[LONGO_WRAP_CACHE_LINES];
        int lens[LONGO_WRAP_CACHE_LINES];
    } wrap_cache;

    Sound sounds[7];
    bool sound_loaded[7];
    Sound music;
    bool music_loaded;
    /* the sim asked for the looping track at least once; the dispatch
     * then keeps it playing across rooms */
    bool music_requested;
    bool audio_ready;
} LongoRender;

/* Acquires every resource; resolves the asset roots; fails (after
 * releasing the partial acquisition) when a required resource — a
 * render target, a sprite strip, the tile atlas or the digits font —
 * is missing.  Audio is optional and degrades with a warning. */
bool longo_render_init(LongoRender *render, const char *asset_root);
/* Releases everything init acquired and zeroes the struct: safe on a
 * partially initialised struct (init's failure paths call it) and
 * idempotent on an already-shut-down one. */
void longo_render_shutdown(LongoRender *render);

/* Full draw phase: shadow surface, application surface and GUI pass.
 * Replays the view items the object scripts pushed. */
void longo_render_frame(LongoRender *render, const SimWorld *world);

/* Audio dispatch for the sounds the simulation queued this tick. */
void longo_render_dispatch_sounds(LongoRender *render, SimWorld *world);

#endif /* LONGO_RENDER_H */
