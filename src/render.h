#ifndef LONGO_RENDER_H
#define LONGO_RENDER_H

#include <stdbool.h>

#include "raylib.h"
#include "game.h"

#define LONGO_LOGICAL_WIDTH 304
#define LONGO_LOGICAL_HEIGHT 208
#define LONGO_WINDOW_SCALE 4

#define LONGO_BITMAP_FONT_GLYPHS 128

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

    /* application surface (GameMaker application_surface) */
    RenderTexture2D app_surface;
    /* GUI canvas: bloom composite + Draw GUI events */
    RenderTexture2D gui_surface;
    /* global.shadow_surf */
    RenderTexture2D shadow_surface;
    bool shadow_surface_ready;
    /* bloom scratch surfaces (obj_bloom_appsrf srf_ping/srf_pong) */
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

    Texture2D tileset1;       /* backgrounds/TileSet1.png (Tiles_3 layers) */
    bool tileset1_loaded;
    Texture2D ground_tileset; /* backgrounds/GroundTileSet.png (Tiles_1) */
    bool ground_tileset_loaded;

    LongoBitmapFont font_longo;       /* LongoFont */
    LongoBitmapFont font_longo_bold;  /* LongoFontBold */
    LongoBitmapFont font_digits;      /* FontDigits */

    Sound sounds[7];
    bool sound_loaded[7];
    Sound music;
    bool music_loaded;
    bool music_playing;
    bool audio_ready;
} LongoRender;

bool longo_render_init(LongoRender *render, const char *asset_root);
void longo_render_shutdown(LongoRender *render);

/* Full GameMaker draw phase: application surface, GUI pass and present. */
void longo_render_frame(LongoRender *render, const LongoWorld *world);

/* Audio dispatch for the sounds the simulation queued this tick. */
void longo_render_dispatch_sounds(LongoRender *render, LongoWorld *world);

#endif /* LONGO_RENDER_H */
