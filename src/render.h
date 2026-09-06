#ifndef LONGO_RENDER_H
#define LONGO_RENDER_H

#include <stdbool.h>

#include "raylib.h"
#include "core/world.h"
#include "core/view.h"

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

typedef struct LongoRender {
    char asset_root[512];

    /* the game world, drawn at the 304x208 logical resolution */
    RenderTexture2D app_surface;
    /* GUI canvas: world composite + UI items */
    RenderTexture2D gui_surface;
    /* per-frame shadow silhouette, composited at low alpha */
    RenderTexture2D shadow_surface;
    bool shadow_surface_ready;

    Texture2D sprites[32];
    bool sprite_loaded[32];

    Texture2D tileset1; /* backgrounds/TileSet1.png (the Tiles_3 layer) */
    bool tileset1_loaded;

    LongoDigitsFont font_digits; /* pixel digits (house counter) */

    Sound sounds[7];
    bool sound_loaded[7];
    Sound music;
    bool music_loaded;
    bool music_playing;
    bool audio_ready;
} LongoRender;

bool longo_render_init(LongoRender *render, const char *asset_root);
void longo_render_shutdown(LongoRender *render);

/* Full draw phase: shadow surface, application surface and GUI pass.
 * Replays the view items the object scripts pushed. */
void longo_render_frame(LongoRender *render, const SimWorld *world);

/* Audio dispatch for the sounds the simulation queued this tick. */
void longo_render_dispatch_sounds(LongoRender *render, SimWorld *world);

#endif /* LONGO_RENDER_H */
