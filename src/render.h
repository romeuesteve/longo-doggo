#ifndef LONGO_RENDER_H
#define LONGO_RENDER_H

#include <stdbool.h>

#include "raylib.h"
#include "core/world.h"
#include "core/view.h"

#define LONGO_LOGICAL_WIDTH 304
#define LONGO_LOGICAL_HEIGHT 208
#define LONGO_WINDOW_SCALE 4

#define LONGO_BITMAP_FONT_GLYPHS 128

/* The pixelated bitmap digits font, used for the in-world house counter
 * so it stays pixelated with the rest of the game. */
typedef struct LongoBitmapGlyph {
    Rectangle source;
    int advance;
    int offset;
    bool present;
} LongoBitmapGlyph;

typedef struct LongoBitmapFont {
    Texture2D texture;
    LongoBitmapGlyph glyphs[LONGO_BITMAP_FONT_GLYPHS];
    int em_size;
    int line_height;
    bool loaded;
} LongoBitmapFont;

typedef struct LongoRender {
    char asset_root[512];

    /* the game world, drawn at the 304x208 logical resolution */
    RenderTexture2D app_surface;
    /* GUI canvas: bloom composite + UI items */
    RenderTexture2D gui_surface;
    /* per-frame shadow silhouette, composited at low alpha */
    RenderTexture2D shadow_surface;
    bool shadow_surface_ready;
    /* bloom scratch surfaces for the ping-pong blur */
    RenderTexture2D bloom_ping;
    RenderTexture2D bloom_pong;

    Shader bloom_lum_shader;
    Shader blur_shader;
    Shader bloom_blend_shader;
    int lum_threshold_loc;
    int lum_range_loc;
    int blur_steps_loc;
    int blur_sigma_loc;
    int blur_vector_loc;
    int blur_texel_loc;
    int blend_intensity_loc;
    int blend_bloom_tex_loc;

    Texture2D sprites[32];
    bool sprite_loaded[32];

    Texture2D tileset1; /* backgrounds/TileSet1.png (the Tiles_3 layer) */
    bool tileset1_loaded;

    LongoBitmapFont font_digits; /* pixel digits (house counter) */

    Sound sounds[7];
    bool sound_loaded[7];
    Sound music;
    bool music_loaded;
    bool music_playing;
    bool audio_ready;
} LongoRender;

bool longo_render_init(LongoRender *render, const char *asset_root);
void longo_render_shutdown(LongoRender *render);

/* Full draw phase: shadow surface, application surface, bloom and GUI
 * pass.  Replays the view items the object scripts pushed. */
void longo_render_frame(LongoRender *render, const SimWorld *world);

/* Audio dispatch for the sounds the simulation queued this tick. */
void longo_render_dispatch_sounds(LongoRender *render, SimWorld *world);

#endif /* LONGO_RENDER_H */
