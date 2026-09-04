/*
 * Longo Doggo renderer.
 *
 * Reproduces the recovered GameMaker Draw events on top of raylib:
 *   - application surface at 304x208 (the original surface_resize size)
 *   - room background layer + tile layers + entities depth-sorted
 *     (higher depth first, ties by creation order)
 *   - global.shadow_surf rebuilt per frame, composited at 0.2 alpha
 *   - GUI pass at 304x208 GUI space (GameMaker's Draw GUI layer):
 *     obj_bloom_appsrf's Draw GUI Begin bloom composite first, then the
 *     tutorial dialogue boxes and transition level wipes
 *
 * All positions come from the presentation layer (eased visuals); all
 * game state comes from the simulation.
 *
 * Render targets never nest: the shadow surface, application surface,
 * bloom ping-pong passes and GUI surface are all filled top-level.
 */
#include "render.h"
#include "room_tiles.h"
#include "sprites.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_RL 3.14159265358979f

/* --------------------------------------------------------------- */
/* Asset loading                                                     */
/* --------------------------------------------------------------- */

static bool file_exists(const char *path)
{
    return path != NULL && FileExists(path);
}

static void make_asset_path(const LongoRender *render, const char *relative,
                            char *out, size_t out_size)
{
    char tmp[1024];
    if (render->asset_root[0] != '\0') {
        snprintf(tmp, sizeof(tmp), "%s/%s", render->asset_root, relative);
        snprintf(out, out_size, "%s", tmp);
    } else {
        snprintf(out, out_size, "%s", relative);
    }
}

static void select_asset_root(LongoRender *render, const char *requested)
{
    static const char *candidates[] = {
        "assets/exported-assets",
        "assets",
        "recovered/exported-assets",
        "../assets/exported-assets",
        "../recovered/exported-assets",
        NULL
    };
    char probe[1024];
    render->asset_root[0] = '\0';
    if (requested != NULL && requested[0] != '\0') {
        snprintf(probe, sizeof(probe), "%s/sprites/sprDogDown/sprDogDown_0.png",
                 requested);
        if (file_exists(probe)) {
            snprintf(render->asset_root, sizeof(render->asset_root), "%s",
                     requested);
            return;
        }
    }
    for (int i = 0; candidates[i] != NULL; ++i) {
        snprintf(probe, sizeof(probe), "%s/sprites/sprDogDown/sprDogDown_0.png",
                 candidates[i]);
        if (file_exists(probe)) {
            snprintf(render->asset_root, sizeof(render->asset_root), "%s",
                     candidates[i]);
            return;
        }
    }
}

/* Load every animation frame of one sprite into a horizontal strip so the
 * GameMaker frame index maps to a source rectangle.  The exported frames
 * keep GameMaker's padded canvas, so all frames share frame 0's size. */
static bool load_sprite(LongoRender *render, LongoSprite sprite)
{
    const char *name = longo_sprite_name(sprite);
    int frames = longo_sprite_frames(sprite);
    char path[1024];
    Image first;
    Image strip;
    Texture2D tex;
    int fw, fh;

    if (sprite == LONGO_SPR_NONE || name == NULL || frames <= 0) return false;
    snprintf(path, sizeof(path), "sprites/%s/%s_0.png", name, name);
    make_asset_path(render, path, path, sizeof(path));
    if (!file_exists(path)) return false;
    first = LoadImage(path);
    if (first.data == NULL) return false;
    fw = first.width;
    fh = first.height;
    UnloadImage(first);

    strip = GenImageColor(fw * frames, fh, BLANK);
    for (int i = 0; i < frames; i++) {
        Image frame_img;
        char fpath[1024];
        snprintf(fpath, sizeof(fpath), "sprites/%s/%s_%d.png", name, name, i);
        make_asset_path(render, fpath, fpath, sizeof(fpath));
        if (!file_exists(fpath)) break;
        frame_img = LoadImage(fpath);
        if (frame_img.data == NULL) break;
        ImageDraw(&strip, frame_img,
                  (Rectangle){ 0, 0, (float)frame_img.width,
                               (float)frame_img.height },
                  (Rectangle){ (float)(i * fw), 0, (float)frame_img.width,
                               (float)frame_img.height },
                  WHITE);
        UnloadImage(frame_img);
    }
    tex = LoadTextureFromImage(strip);
    UnloadImage(strip);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    render->sprites[sprite] = tex;
    render->sprite_loaded[sprite] = tex.id != 0;
    return render->sprite_loaded[sprite];
}

static bool load_bitmap_font(LongoRender *render, LongoBitmapFont *font,
                             const char *image_name, const char *glyph_name)
{
    char image_path[1024];
    char glyph_path[1024];
    char *csv;
    char *line;
    bool first_line = true;

    if (render == NULL || font == NULL) return false;
    memset(font, 0, sizeof(*font));
    font->em_size = 8;
    font->line_height = 12;

    snprintf(image_path, sizeof(image_path), "fonts/%s", image_name);
    make_asset_path(render, image_path, image_path, sizeof(image_path));
    if (!file_exists(image_path)) return false;
    font->texture = LoadTexture(image_path);
    if (font->texture.id == 0) return false;
    SetTextureFilter(font->texture, TEXTURE_FILTER_POINT);

    make_asset_path(render, glyph_name, glyph_path, sizeof(glyph_path));
    csv = LoadFileText(glyph_path);
    if (csv == NULL) {
        UnloadTexture(font->texture);
        font->texture = (Texture2D){ 0 };
        return false;
    }
    line = csv;
    while (line != NULL && *line != '\0') {
        char *end = strchr(line, '\n');
        int character, sx, sy, sw, sh, shift, offset;
        if (end != NULL) *end = '\0';
        if (first_line) {
            int em_size;
            if (sscanf(line, "\"%*[^\"]\";%d", &em_size) == 1 && em_size > 0)
                font->em_size = em_size;
            first_line = false;
        } else if (sscanf(line, "%d;%d;%d;%d;%d;%d;%d", &character, &sx, &sy,
                          &sw, &sh, &shift, &offset) == 7 &&
                   character >= 0 && character < LONGO_BITMAP_FONT_GLYPHS) {
            LongoBitmapGlyph *glyph = &font->glyphs[character];
            glyph->source = (Rectangle){ (float)sx, (float)sy, (float)sw,
                                         (float)sh };
            glyph->advance = shift;
            glyph->offset = offset;
            glyph->present = true;
            if (sh > font->line_height) font->line_height = sh;
        }
        if (end == NULL) break;
        line = end + 1;
    }
    UnloadFileText(csv);
    font->loaded = true;
    return true;
}

static Sound load_sound_relative(const LongoRender *render, const char *file,
                                 bool *loaded)
{
    char path[1024];
    snprintf(path, sizeof(path), "audio/audiogroup_default/%s", file);
    make_asset_path(render, path, path, sizeof(path));
    if (!file_exists(path) || !render->audio_ready) {
        if (loaded) *loaded = false;
        return (Sound){ 0 };
    }
    Sound sound = LoadSound(path);
    if (loaded) *loaded = sound.frameCount != 0;
    return sound;
}

bool longo_render_init(LongoRender *render, const char *asset_root)
{
    if (render == NULL) return false;
    memset(render, 0, sizeof(*render));
    select_asset_root(render, asset_root);

    render->app_surface = LoadRenderTexture(LONGO_LOGICAL_WIDTH,
                                            LONGO_LOGICAL_HEIGHT);
    render->gui_surface = LoadRenderTexture(LONGO_LOGICAL_WIDTH,
                                            LONGO_LOGICAL_HEIGHT);
    render->shadow_surface = LoadRenderTexture(LONGO_LOGICAL_WIDTH,
                                               LONGO_LOGICAL_HEIGHT);
    render->bloom_ping = LoadRenderTexture(LONGO_LOGICAL_WIDTH,
                                           LONGO_LOGICAL_HEIGHT);
    render->bloom_pong = LoadRenderTexture(LONGO_LOGICAL_WIDTH,
                                           LONGO_LOGICAL_HEIGHT);
    SetTextureFilter(render->app_surface.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(render->gui_surface.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(render->shadow_surface.texture, TEXTURE_FILTER_POINT);
    /* GameMaker enables linear filtering for the bloom chain only. */
    SetTextureFilter(render->bloom_ping.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(render->bloom_pong.texture, TEXTURE_FILTER_BILINEAR);

    render->bloom_lum_shader = LoadShaderFromMemory(NULL,
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform float threshold;\n"
        "uniform float range;\n"
        "void main() {\n"
        "    vec4 c = texture(texture0, fragTexCoord);\n"
        "    float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
        "    float f = smoothstep(threshold, threshold + range, lum);\n"
        "    fragColor = vec4(c.rgb * f, c.a * f);\n"
        "}\n");
    render->blur_shader = LoadShaderFromMemory(NULL,
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform vec2 texel_size;\n"
        "uniform vec2 blur_vector;\n"
        "uniform float blur_steps;\n"
        "uniform float sigma;\n"
        "void main() {\n"
        "    vec4 total = vec4(0.0);\n"
        "    float weights = 0.0;\n"
        "    int n = int(blur_steps);\n"
        "    for (int i = -n; i <= n; i++) {\n"
        "        float w = exp(-0.5 * float(i * i) / (sigma * sigma + 1e-6));\n"
        "        total += texture(texture0, fragTexCoord +\n"
        "                   blur_vector * texel_size * float(i)) * w;\n"
        "        weights += w;\n"
        "    }\n"
        "    fragColor = total / weights;\n"
        "}\n");
    render->bloom_blend_shader = LoadShaderFromMemory(NULL,
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D bloom_texture;\n"
        "uniform float bloom_intensity;\n"
        "void main() {\n"
        "    vec4 base = texture(texture0, fragTexCoord);\n"
        "    vec4 bloom = texture(bloom_texture, fragTexCoord);\n"
        "    fragColor = vec4(base.rgb + bloom.rgb * bloom_intensity, base.a);\n"
        "}\n");
    render->lum_threshold_loc = GetShaderLocation(render->bloom_lum_shader,
                                                  "threshold");
    render->lum_range_loc = GetShaderLocation(render->bloom_lum_shader, "range");
    render->blur_steps_loc = GetShaderLocation(render->blur_shader,
                                               "blur_steps");
    render->blur_sigma_loc = GetShaderLocation(render->blur_shader, "sigma");
    render->blur_vector_loc = GetShaderLocation(render->blur_shader,
                                                "blur_vector");
    render->blur_texel_loc = GetShaderLocation(render->blur_shader,
                                               "texel_size");
    render->blend_intensity_loc = GetShaderLocation(render->bloom_blend_shader,
                                                    "bloom_intensity");
    render->blend_bloom_tex_loc = GetShaderLocation(render->bloom_blend_shader,
                                                    "bloom_texture");

    for (int s = 1; s < 32; s++) {
        if (longo_sprite_name(s) != NULL) load_sprite(render, (LongoSprite)s);
    }

    {
        char path[1024];
        snprintf(path, sizeof(path), "backgrounds/TileSet1.png");
        make_asset_path(render, path, path, sizeof(path));
        if (file_exists(path)) {
            render->tileset1 = LoadTexture(path);
            SetTextureFilter(render->tileset1, TEXTURE_FILTER_POINT);
            render->tileset1_loaded = render->tileset1.id != 0;
        }
        snprintf(path, sizeof(path), "backgrounds/GroundTileSet.png");
        make_asset_path(render, path, path, sizeof(path));
        if (file_exists(path)) {
            render->ground_tileset = LoadTexture(path);
            SetTextureFilter(render->ground_tileset, TEXTURE_FILTER_POINT);
            render->ground_tileset_loaded = render->ground_tileset.id != 0;
        }
    }

    load_bitmap_font(render, &render->font_longo, "LongoFont.png",
                     "fonts/glyphs_LongoFont.csv");
    load_bitmap_font(render, &render->font_longo_bold, "LongoFontBold.png",
                     "fonts/glyphs_LongoFontBold.csv");
    load_bitmap_font(render, &render->font_digits, "FontDigits.png",
                     "fonts/glyphs_FontDigits.csv");

    if (!IsAudioDeviceReady()) InitAudioDevice();
    render->audio_ready = IsAudioDeviceReady();
    render->sounds[LONGO_SND_BARK] = load_sound_relative(render, "snd_bark.wav",
        &render->sound_loaded[LONGO_SND_BARK]);
    render->sounds[LONGO_SND_BUTTON] = load_sound_relative(render,
        "snd_button.wav", &render->sound_loaded[LONGO_SND_BUTTON]);
    render->sounds[LONGO_SND_POOF] = load_sound_relative(render, "snd_poof.wav",
        &render->sound_loaded[LONGO_SND_POOF]);
    render->sounds[LONGO_SND_PUSHED] = load_sound_relative(render,
        "snd_pushed.wav", &render->sound_loaded[LONGO_SND_PUSHED]);
    render->sounds[LONGO_SND_WIN] = load_sound_relative(render, "snd_win.wav",
        &render->sound_loaded[LONGO_SND_WIN]);
    render->sounds[LONGO_SND_WRONG] = load_sound_relative(render,
        "snd_wrong.wav", &render->sound_loaded[LONGO_SND_WRONG]);
    render->music = load_sound_relative(render, "snd_placeholder.wav",
                                        &render->music_loaded);
    return render->app_surface.id != 0;
}

void longo_render_shutdown(LongoRender *render)
{
    if (render == NULL) return;
    for (int s = 0; s < 32; s++) {
        if (render->sprite_loaded[s]) UnloadTexture(render->sprites[s]);
    }
    if (render->font_longo.loaded) UnloadTexture(render->font_longo.texture);
    if (render->font_longo_bold.loaded)
        UnloadTexture(render->font_longo_bold.texture);
    if (render->font_digits.loaded) UnloadTexture(render->font_digits.texture);
    if (render->tileset1_loaded) UnloadTexture(render->tileset1);
    if (render->ground_tileset_loaded) UnloadTexture(render->ground_tileset);
    for (int s = 0; s < 7; s++) {
        if (render->sound_loaded[s]) UnloadSound(render->sounds[s]);
    }
    if (render->music_loaded) UnloadSound(render->music);
    UnloadRenderTexture(render->app_surface);
    UnloadRenderTexture(render->gui_surface);
    UnloadRenderTexture(render->shadow_surface);
    UnloadRenderTexture(render->bloom_ping);
    UnloadRenderTexture(render->bloom_pong);
    UnloadShader(render->bloom_lum_shader);
    UnloadShader(render->blur_shader);
    UnloadShader(render->bloom_blend_shader);
    memset(render, 0, sizeof(*render));
}

/* --------------------------------------------------------------- */
/* GameMaker draw primitives                                         */
/* --------------------------------------------------------------- */

static Color make_color_rgb(int r, int g, int b)
{
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
}

/* draw_sprite_ext(): (x, y) is the sprite origin position. */
static void draw_sprite_origin(LongoRender *render, LongoSprite sprite,
                               int frame, float x, float y, float xscale,
                               float yscale, float rotation, Color tint,
                               float alpha)
{
    if (sprite <= LONGO_SPR_NONE || !render->sprite_loaded[sprite]) return;
    int frames = longo_sprite_frames(sprite);
    if (frames <= 0) return;
    int fw = render->sprites[sprite].width / frames;
    int fh = render->sprites[sprite].height;
    if (frame < 0) frame = 0;
    if (frame >= frames) frame %= frames;
    int ox = longo_sprite_origin_x(sprite);
    int oy = longo_sprite_origin_y(sprite);

    Rectangle source = { (float)(frame * fw), 0.0f, (float)fw, (float)fh };
    Vector2 origin = { ox * xscale, oy * yscale };
    Rectangle dest = { x, y, (float)fw * xscale, (float)fh * yscale };
    Color c = tint;
    c.a = (unsigned char)(255.0f * alpha + 0.5f);
    if (rotation != 0.0f || xscale != 1.0f || yscale != 1.0f) {
        DrawTexturePro(render->sprites[sprite], source, dest, origin, rotation,
                       c);
    } else {
        DrawTextureRec(render->sprites[sprite], source,
                       (Vector2){ x - origin.x, y - origin.y }, c);
    }
}

/* draw_sprite_part_ext(): source region in frame coordinates, clipped to
 * the frame bounds like GameMaker's draw_sprite_part*; no origin offset. */
static void draw_sprite_part_ext(LongoRender *render, LongoSprite sprite,
                                 int frame, int src_x, int src_y, int src_w,
                                 int src_h, float x, float y, float xscale,
                                 float yscale, Color tint, float alpha)
{
    if (sprite <= LONGO_SPR_NONE || !render->sprite_loaded[sprite]) return;
    int frames = longo_sprite_frames(sprite);
    if (frames <= 0) return;
    int fw = render->sprites[sprite].width / frames;
    int fh = render->sprites[sprite].height;
    if (frame < 0) frame = 0;
    if (frame >= frames) frame %= frames;
    if (src_x < 0) src_x = 0;
    if (src_y < 0) src_y = 0;
    if (src_x + src_w > fw) src_w = fw - src_x;
    if (src_y + src_h > fh) src_h = fh - src_y;
    if (src_w <= 0 || src_h <= 0) return;
    Rectangle source = { (float)(frame * fw + src_x), (float)src_y,
                         (float)src_w, (float)src_h };
    Rectangle dest = { x, y, (float)src_w * xscale, (float)src_h * yscale };
    Color c = tint;
    c.a = (unsigned char)(255.0f * alpha + 0.5f);
    DrawTexturePro(render->sprites[sprite], source, dest, (Vector2){ 0, 0 },
                   0.0f, c);
}

static int frame_of(float image_index)
{
    int frame = (int)image_index;
    return frame < 0 ? 0 : frame;
}

/* draw_line_width_color(): gradient c1 -> c2. */
static void draw_line_width_color(Vector2 a, Vector2 b, float width, Color c1,
                                  Color c2)
{
    if (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b) {
        DrawLineEx(a, b, width, c1);
        return;
    }
    Vector2 mid = { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
    Color half1 = { c1.r, c1.g, c1.b,
                    (unsigned char)((c1.a + c2.a) / 2) };
    Color half2 = { (unsigned char)((c1.r + c2.r) / 2),
                    (unsigned char)((c1.g + c2.g) / 2),
                    (unsigned char)((c1.b + c2.b) / 2), c2.a };
    DrawLineEx(a, mid, width, half1);
    DrawLineEx(mid, b, width, half2);
}

/* draw_circle_color(): vertical gradient c1 (top) -> c2 (bottom). */
static void draw_circle_color(float x, float y, float radius, Color c1,
                              Color c2)
{
    if (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b) {
        DrawCircleV((Vector2){ x, y }, radius, c1);
        return;
    }
    DrawCircleGradient((Vector2){ x, y }, radius, c1, c2);
}

static float wave_calc(float a, float b, float period, float phase,
                       double time_ms)
{
    return longo_wave(a, b, period, phase, time_ms);
}

static float point_direction(float x1, float y1, float x2, float y2)
{
    float dir = atan2f(-(y2 - y1), x2 - x1) * (180.0f / PI_RL);
    if (dir < 0) dir += 360.0f;
    return dir;
}

static float lengthdir_x(float len, float dir)
{
    return cosf(dir * (PI_RL / 180.0f)) * len;
}

static float lengthdir_y(float len, float dir)
{
    return -sinf(dir * (PI_RL / 180.0f)) * len;
}

/* --------------------------------------------------------------- */
/* Bitmap text                                                       */
/* --------------------------------------------------------------- */

static const LongoBitmapFont *font_by_asset(const LongoRender *render,
                                            int font_id)
{
    /* GameMaker font assets: 0 LongoFontBold, 1 LongoFont, 2 FontDigits. */
    if (font_id == 0 && render->font_longo_bold.loaded)
        return &render->font_longo_bold;
    if (font_id == 1 && render->font_longo.loaded) return &render->font_longo;
    if (font_id == 2 && render->font_digits.loaded) return &render->font_digits;
    if (render->font_longo.loaded) return &render->font_longo;
    if (render->font_longo_bold.loaded) return &render->font_longo_bold;
    return NULL;
}

static float text_width_scaled(const LongoBitmapFont *font, const char *text,
                               float scale)
{
    float line_width = 0.0f, maximum = 0.0f;
    if (font == NULL || text == NULL) return 0.0f;
    for (const unsigned char *c = (const unsigned char *)text; *c; ++c) {
        if (*c == '\n') {
            if (line_width > maximum) maximum = line_width;
            line_width = 0.0f;
            continue;
        }
        if (*c < LONGO_BITMAP_FONT_GLYPHS && font->glyphs[*c].present)
            line_width += font->glyphs[*c].advance * scale;
        else
            line_width += 4.0f * scale;
    }
    if (line_width > maximum) maximum = line_width;
    return maximum;
}

static void draw_glyph_run(const LongoBitmapFont *font, const char *text,
                           float x, float y, float scale, Color color)
{
    for (const unsigned char *c = (const unsigned char *)text; *c; ++c) {
        if (*c == '\n') break;
        if (*c >= LONGO_BITMAP_FONT_GLYPHS || !font->glyphs[*c].present) {
            x += 4.0f * scale;
            continue;
        }
        const LongoBitmapGlyph *g = &font->glyphs[*c];
        Rectangle dest = { x, y + g->offset * scale,
                           g->source.width * scale, g->source.height * scale };
        DrawTexturePro(font->texture, g->source, dest, (Vector2){ 0, 0 }, 0.0f,
                       color);
        x += g->advance * scale;
    }
}

static void draw_text_left(const LongoBitmapFont *font, const char *text,
                           float x, float y, float scale, Color color)
{
    if (font == NULL || !font->loaded || text == NULL) return;
    draw_glyph_run(font, text, x, y, scale, color);
}

static void draw_text_centered(const LongoBitmapFont *font, const char *text,
                               float x, float y, float scale, Color color)
{
    if (font == NULL || !font->loaded || text == NULL) return;
    float w = text_width_scaled(font, text, scale);
    draw_glyph_run(font, text, x - w * 0.5f, y, scale, color);
}

/* draw_text_ext_transformed() with halign center: word wrap at `width`,
 * line separation `sep`, uniform scale.  Handles embedded newlines. */
static void draw_text_ext_centered(const LongoBitmapFont *font,
                                   const char *text, float x, float y,
                                   float sep, float width, float scale,
                                   Color color)
{
    if (font == NULL || !font->loaded || text == NULL) return;
    const char *p = text;
    float line_y = y;
    while (*p != '\0') {
        char line[512];
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, p, len);
        line[len] = '\0';

        const char *cursor = line;
        while (*cursor != '\0') {
            char out[512];
            out[0] = '\0';
            while (*cursor != '\0') {
                while (*cursor == ' ') cursor++;
                if (*cursor == '\0') break;
                const char *w_end = cursor;
                while (*w_end != '\0' && *w_end != ' ') w_end++;
                char word[256];
                size_t wl = (size_t)(w_end - cursor);
                if (wl >= sizeof(word)) wl = sizeof(word) - 1;
                memcpy(word, cursor, wl);
                word[wl] = '\0';
                char probe[512];
                if (out[0] != '\0')
                    snprintf(probe, sizeof(probe), "%s %s", out, word);
                else
                    snprintf(probe, sizeof(probe), "%s", word);
                if (text_width_scaled(font, probe, scale) > width &&
                    out[0] != '\0') {
                    break;
                }
                snprintf(out, sizeof(out), "%s", probe);
                cursor = w_end;
            }
            float w = text_width_scaled(font, out, scale);
            draw_glyph_run(font, out, x - w * 0.5f, line_y, scale, color);
            line_y += sep * scale;
            if (*cursor == ' ') cursor++;
        }
        p += len;
        if (nl != NULL) p++;
    }
}

/* --------------------------------------------------------------- */
/* Entity draws (room space)                                          */
/* --------------------------------------------------------------- */

static LongoSprite dog_face_sprite(int dir)
{
    switch (dir) {
    case 0: return LONGO_SPR_DOGDOWN;
    case 90: return LONGO_SPR_DOGRIGHT;
    case 180: return LONGO_SPR_DOGUP;
    default: return LONGO_SPR_DOGLEFT;
    }
}

static int flower_frame(const Pres *pres)
{
    return frame_of(pres->flower_clock);
}

/* oDog Draw event: the visible dog is vector art over the part chain,
 * plus the head sprite.  Part order in the sim (chain[0] nearest the
 * head) matches the original instance order. */
static void draw_dog_world(LongoRender *render, const SimWorld *world,
                           const Pres *pres)
{
    const Color body = make_color_rgb(153, 108, 53);
    const Color outline = make_color_rgb(107, 61, 49);
    const SimDog *dog = &world->dog;
    int tail_frame = flower_frame(pres);

    /* pass 1: legs, outline body, outline head circle */
    for (int i = 0; i < dog->length; i++) {
        float px = pres->part_x[i];
        float py = pres->part_y[i];
        float fx = i == 0 ? pres->dog_x : pres->part_x[i - 1];
        float fy = i == 0 ? pres->dog_y : pres->part_y[i - 1];
        int is_first = (dog->pflag[i] & SIM_PART_FIRST) != 0;
        int legs = (dog->pflag[i] & SIM_PART_LEGS) != 0;
        if (legs) {
            float angle_amp = pres_part_legs_angle(pres, i);
            float legs_wave = wave_calc(-angle_amp, angle_amp, 0.2f, 0,
                                        pres->time_ms);
            float dir = (point_direction(px, py, fx, fy) - 90.0f) +
                        (180.0f * is_first);
            float leglength = 6.0f + (is_first ? 3.0f : 0.0f);
            for (int leg = 0; leg < 2; leg++) {
                float angle = (leg == 0 ? -45.0f : 225.0f) + legs_wave + dir;
                Vector2 a = { px, py };
                Vector2 b = { px + lengthdir_x(leglength, angle),
                              py + lengthdir_y(leglength, angle) };
                draw_line_width_color(a, b, 2.0f, outline, body);
                draw_circle_color(b.x, b.y, 3.0f, body, outline);
            }
        }
        draw_circle_color(px - 1.0f, py - 1.0f, 5.0f, outline, outline);
        draw_line_width_color((Vector2){ px - 1.0f, py - 1.0f },
                              (Vector2){ fx - 1.0f, fy - 1.0f }, 10.0f,
                              outline, outline);
        if (is_first) {
            draw_circle_color(pres->dog_x, pres->dog_y - 1.0f, 5.0f, outline,
                              outline);
        }
    }

    /* pass 2: fill body and tail sprite */
    for (int i = 0; i < dog->length; i++) {
        float px = pres->part_x[i];
        float py = pres->part_y[i];
        float fx = i == 0 ? pres->dog_x : pres->part_x[i - 1];
        float fy = i == 0 ? pres->dog_y : pres->part_y[i - 1];
        int legs = (dog->pflag[i] & SIM_PART_LEGS) != 0;
        draw_circle_color(px - 1.0f, py - 1.0f, 4.0f, body, body);
        draw_line_width_color((Vector2){ px - 1.0f, py - 1.0f },
                              (Vector2){ fx - 1.0f, fy - 1.0f }, 8.0f, body,
                              body);
        if (legs && !(dog->pflag[i] & SIM_PART_FIRST)) {
            draw_sprite_origin(render, LONGO_SPR_DOGTAIL, tail_frame,
                               px - 1.0f, py - 3.0f, 1.0f, 1.0f, 0.0f, WHITE,
                               1.0f);
        }
    }

    draw_sprite_origin(render, dog_face_sprite(dog->dir), flower_frame(pres),
                       pres->dog_x, pres->dog_y, 1.0f, 1.0f, 0.0f, WHITE,
                       1.0f);
}

/* --------------------------------------------------------------- */
/* oShadows Draw event (global.shadow_surf)                           */
/* --------------------------------------------------------------- */

static void build_shadow_surface(LongoRender *render, const SimWorld *world,
                                 const Pres *pres)
{
    const SimDog *dog = &world->dog;
    Color black = BLACK;

    BeginTextureMode(render->shadow_surface);
    ClearBackground(BLANK);

    for (int i = 0; i < dog->length; i++) {
        float px = pres->part_x[i];
        float py = pres->part_y[i];
        float fx = i == 0 ? pres->dog_x : pres->part_x[i - 1];
        float fy = i == 0 ? pres->dog_y : pres->part_y[i - 1];
        int is_first = (dog->pflag[i] & SIM_PART_FIRST) != 0;
        int legs = (dog->pflag[i] & SIM_PART_LEGS) != 0;
        if (legs) {
            float angle_amp = pres_part_legs_angle(pres, i);
            float legs_wave = wave_calc(-angle_amp, angle_amp, 0.2f, 0,
                                        pres->time_ms);
            float dir = (point_direction(px, py, fx, fy) - 90.0f) +
                        (180.0f * is_first);
            float leglength = 6.0f + (is_first ? 3.0f : 0.0f);
            for (int leg = 0; leg < 2; leg++) {
                float angle = (leg == 0 ? -45.0f : 225.0f) + legs_wave + dir;
                Vector2 a = { px, py + 5.0f };
                Vector2 b = { px + lengthdir_x(leglength, angle),
                              py + 5.0f + lengthdir_y(leglength, angle) };
                DrawLineEx(a, b, 2.0f, black);
                DrawCircleV(b, 3.0f, black);
            }
        }
        DrawCircleV((Vector2){ px - 1.0f, py + 4.0f }, 5.0f, black);
        DrawLineEx((Vector2){ px - 1.0f, py + 4.0f },
                   (Vector2){ fx - 1.0f, fy + 4.0f }, 10.0f, black);
    }

    if (dog->alive) {
        draw_sprite_origin(render, dog_face_sprite(dog->dir),
                           flower_frame(pres), pres->dog_x, pres->dog_y + 5.0f,
                           1.0f, 1.0f, 0.0f, black, 1.0f);
    }
    for (int i = 0; i < world->apple_count; i++) {
        if (!world->apples[i].alive) continue;
        draw_sprite_origin(render, LONGO_SPR_APPLE, frame_of(pres->apple_clock),
                           (float)(sim_cell_x(world->apples[i].cell) * SIM_CELL),
                           (float)(sim_cell_y(world->apples[i].cell) * SIM_CELL + 7.0f),
                           1.0f, 0.6f, 0.0f, black, 1.0f);
    }
    for (int i = 0; i < world->skull_count; i++) {
        if (!world->skulls[i].alive) continue;
        draw_sprite_origin(render, LONGO_SPR_SKULL, frame_of(pres->pear_clock),
                           (float)(sim_cell_x(world->skulls[i].cell) * SIM_CELL),
                           (float)(sim_cell_y(world->skulls[i].cell) * SIM_CELL + 7.0f),
                           1.0f, 0.6f, 0.0f, black, 1.0f);
    }
    for (int i = 0; i < PRES_MAX_FLIES; i++) {
        const PresButterfly *fly = &pres->flies[i];
        if (!fly->alive) continue;
        draw_sprite_origin(render, LONGO_SPR_FLY, frame_of(pres->apple_clock),
                           fly->x, fly->y + 16.0f, 1.0f, 0.6f, 0.0f, black,
                           1.0f);
    }
    if (world->goal.alive) {
        /* the goal instance sits at the cell column centre, one cell down */
        float gx = (float)(sim_cell_x(world->goal.cell) * SIM_CELL + 8);
        float gy = (float)(sim_cell_y(world->goal.cell) * SIM_CELL + 16);
        draw_sprite_origin(render, LONGO_SPR_HOUSE, flower_frame(pres),
                           gx, gy + 4.0f, 1.0f, 0.5f, 0.0f, black, 1.0f);
    }
    for (int i = 0; i < world->box_count; i++) {
        if (!world->boxes[i].alive) continue;
        draw_sprite_origin(render, LONGO_SPR_BOX, 0, pres->box_x[i],
                           pres->box_y[i] + 5.0f, 1.0f, 1.0f, 0.0f, black,
                           1.0f);
    }
    for (int i = 0; i < world->door_count; i++) {
        if (!world->doors[i].alive) continue;
        draw_sprite_origin(render, LONGO_SPR_DOOR, 0,
                           (float)(sim_cell_x(world->doors[i].cell) * SIM_CELL),
                           (float)(sim_cell_y(world->doors[i].cell) * SIM_CELL + 22.0f),
                           1.0f, -0.4f, 0.0f, black, 1.0f);
    }
    for (int i = 0; i < world->button_count; i++) {
        if (!world->buttons[i].alive) continue;
        const SimButton *b = &world->buttons[i];
        /* zone cells carry the button's bbox: draw at the first cell */
        draw_sprite_origin(render,
                           b->pressed ? LONGO_SPR_BUTTONPRESSED
                                      : LONGO_SPR_BUTTON,
                           0,
                           (float)(sim_cell_x(b->zone[0]) * SIM_CELL),
                           (float)(sim_cell_y(b->zone[0]) * SIM_CELL + 4.0f),
                           1.0f, 1.0f, 0.0f, black, 1.0f);
    }
    if (pres->has_title_decor) {
        float wave1 = wave_calc(0, 8, 2, 0, pres->time_ms);
        float wave2 = wave_calc(0, 8, 2, 0.1f, pres->time_ms);
        draw_sprite_part_ext(render, LONGO_SPR_TITLE, 0, 0, 0, 191, 64,
                             pres->title_x, pres->title_y + wave1 + 85.0f,
                             1.0f, 0.5f, black, 1.0f);
        draw_sprite_part_ext(render, LONGO_SPR_TITLE, 0, 0, 69, 191, 149,
                             pres->title_x,
                             pres->title_y + 48.0f + wave2 + 52.0f, 1.0f, 0.5f,
                             black, 1.0f);
    }
    EndTextureMode();
}

/* --------------------------------------------------------------- */
/* Instance draws (room space)                                        */
/* --------------------------------------------------------------- */

static void draw_smoke_puff(LongoRender *render, const PresSmoke *smoke)
{
    Color cream = make_color_rgb(255, 235, 204);
    Color gold = make_color_rgb(235, 176, 81);
    static const float offsets[5][2] = {
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { 0, 0 }
    };
    for (int i = 0; i < 5; i++) {
        draw_sprite_origin(render, LONGO_SPR_SMOKE, 0,
                           smoke->x + offsets[i][0], smoke->y + offsets[i][1],
                           smoke->scale, smoke->scale, smoke->angle,
                           i == 4 ? cream : gold, 1.0f);
    }
}

/* oGoalUp draw: house sprite, remain number and the pulse scales. */
static void draw_goal(LongoRender *render, const SimWorld *world,
                      const Pres *pres)
{
    const SimGoal *goal = &world->goal;
    const LongoBitmapFont *font = font_by_asset(render, 2);
    /* oGoal instance position: cell column centre, one cell below the top */
    float gx = (float)(sim_cell_x(goal->cell) * SIM_CELL + 8);
    float gy = (float)(sim_cell_y(goal->cell) * SIM_CELL + 16);
    int house_frame = goal->remain <= 0 ? 1 : 0;

    draw_sprite_part_ext(render, LONGO_SPR_HOUSE, house_frame, 0, 0, 64, 44,
                         gx - (32.0f * pres->goal_scale_x),
                         gy - (64.0f * pres->goal_scale_y),
                         pres->goal_scale_x, pres->goal_scale_y, WHITE, 1.0f);
    if (goal->remain > 0) {
        char text[16];
        float wave = wave_calc(-pres->goal_count2 / 50.0f,
                               pres->goal_count2 / 50.0f, 0.35f, 0,
                               pres->time_ms);
        float y = (gy + 1.0f) - 32.0f + wave;
        snprintf(text, sizeof(text), "%d", goal->remain);
        if (font == NULL || !font->loaded) return;
        draw_text_centered(font, text, gx + 1.0f,
                           y - font->line_height * 0.5f, 1.0f,
                           make_color_rgb(128, 0, 0));
        draw_text_centered(font, text, gx, y - font->line_height * 0.5f, 1.0f,
                           make_color_rgb(255, 0, 0));
    }
}

static void draw_title(LongoRender *render, const Pres *pres)
{
    const LongoBitmapFont *font = font_by_asset(render, 0);
    float wave1 = wave_calc(0, 8, 2, 0, pres->time_ms);
    float wave2 = wave_calc(0, 8, 2, 0.1f, pres->time_ms);
    draw_sprite_part_ext(render, LONGO_SPR_TITLE, 0, 0, 0, 191, 64,
                         pres->title_x, pres->title_y + wave1, 1.0f, 1.0f,
                         WHITE, 1.0f);
    draw_sprite_part_ext(render, LONGO_SPR_TITLE, 0, 0, 69, 191, 149,
                         pres->title_x, pres->title_y + 48.0f + wave2 + 2.0f,
                         1.0f, 1.0f, WHITE, 1.0f);
    if (font != NULL && font->loaded) {
        float y = 188.0f + wave1 * 0.5f;
        draw_text_left(font, "Press Any Key to Start", 151.0f, y + 1.0f, 1.0f,
                       make_color_rgb(51, 17, 0));
        draw_text_left(font, "Press Any Key to Start", 150.0f, y, 1.0f,
                       make_color_rgb(255, 235, 204));
    }
}

/* --------------------------------------------------------------- */
/* Tile layers and room compose                                       */
/* --------------------------------------------------------------- */

/* Tile layers hold 8x8 tiles addressed in 12x12 atlas cells. */
static void draw_tile_layer(LongoRender *render, const LongoTileLayer *layer)
{
    Texture2D atlas;
    int columns;
    int tile_count;

    if (layer == NULL || layer->data == NULL) return;
    if (layer->tileset == LONGO_TILESET_GROUND) {
        atlas = render->ground_tileset;
        if (!render->ground_tileset_loaded) return;
        columns = 6;
        tile_count = 42;
    } else {
        atlas = render->tileset1;
        if (!render->tileset1_loaded) return;
        columns = 8;
        tile_count = 64;
    }
    for (int y = 0; y < layer->height; ++y) {
        for (int x = 0; x < layer->width; ++x) {
            unsigned int raw = layer->data[y * layer->width + x];
            int tile_index;
            if (raw == 0u) continue;
            tile_index = (int)(raw & 0x0fffffffu);
            if (tile_index < 0 || tile_index >= tile_count) continue;
            Rectangle source = {
                2.0f + (tile_index % columns) * 12.0f,
                2.0f + (tile_index / columns) * 12.0f, 8.0f, 8.0f
            };
            Vector2 dest = { (float)(layer->offset_x + x * 8),
                             (float)(layer->offset_y + y * 8) };
            DrawTextureRec(atlas, source, dest, WHITE);
        }
    }
}

static const LongoRoomTileMap *tile_map_for(const LongoRoom *room)
{
    for (int i = 0; i < LONGO_ROOM_TILE_COUNT; i++) {
        if (strcmp(longo_room_tile_maps[i].room_name, room->name) == 0)
            return &longo_room_tile_maps[i];
    }
    return NULL;
}

/* --------------------------------------------------------------- */
/* GUI pass                                                           */
/* --------------------------------------------------------------- */

static void draw_tutorial_gui(LongoRender *render, const SimWorld *world,
                              const Pres *pres)
{
    const LongoBitmapFont *font = font_by_asset(render, 1);
    const SimDialogue *dlg = &world->dialogue;
    const SimDbox *box;
    float wave;
    float width;

    if (!dlg->active) return;
    int index = dlg->index;
    if (index < 0) index = 0;
    if (index > 7) index = 7;
    box = &dlg->box[index];
    if (box->text == NULL) return;
    wave = wave_calc(0, 2, 2, 0, pres->time_ms);

    draw_sprite_origin(render, LONGO_SPR_DIALOGUEBOX, 0, box->x,
                       box->y + wave, pres->dlg_scale_x, pres->dlg_scale_y,
                       0.0f, WHITE, 1.0f);
    if (font == NULL || !font->loaded) return;
    float scale = pres->dlg_scale_y * 0.3f;
    width = 30.0f * dlg->base_scale;
    draw_text_ext_centered(font, box->text, box->x + 0.5f,
                           box->y + wave + 0.5f, 12.0f, width, scale,
                           make_color_rgb(255, 196, 101));
    draw_text_ext_centered(font, box->text, box->x, box->y + wave, 12.0f,
                           width, scale, make_color_rgb(84, 64, 32));
}

static void draw_transition_gui(LongoRender *render, const SimWorld *world)
{
    (void)world;
    const SimTransition *trans = &world->trans;
    Color color2 = make_color_rgb(113 - 10, 153 - 10, 61 - 10);
    Color color = make_color_rgb(141 - 10, 199 - 10, 63 - 10);

    if (!trans->active) return;

    if (trans->open_transition) {
        for (int i = 0; i < 4; i++) {
            draw_sprite_origin(render, LONGO_SPR_TRANSITION, 0, trans->x,
                               (float)(i * 64) + 7.0f, 1.0f, 1.0f, 0.0f,
                               color2, 1.0f);
            draw_sprite_origin(render, LONGO_SPR_TRANSITION, 0, trans->x,
                               (float)(i * 64), 1.0f, 1.0f, 0.0f, color, 1.0f);
        }
        if (trans->x + 32.0f > 0.0f) {
            DrawRectangle((int)(trans->x + 32.0f), 0,
                          (int)(304.0f - (trans->x + 32.0f)),
                          LONGO_LOGICAL_HEIGHT, color);
        }
    } else if (trans->close_transition) {
        for (int i = 0; i < 4; i++) {
            draw_sprite_origin(render, LONGO_SPR_TRANSITION, 1, trans->x,
                               (float)(i * 64) + 7.0f, 1.0f, 1.0f, 0.0f,
                               color2, 1.0f);
            draw_sprite_origin(render, LONGO_SPR_TRANSITION, 1, trans->x,
                               (float)(i * 64), 1.0f, 1.0f, 0.0f, color, 1.0f);
        }
        if (trans->x + 32.0f > 0.0f) {
            DrawRectangle(0, 0, (int)(trans->x + 32.0f),
                          LONGO_LOGICAL_HEIGHT, color);
        }
    }

    const LongoBitmapFont *font = font_by_asset(render, 0);
    if (font == NULL || !font->loaded) return;
    char text[128];
    if (trans->room_num <= 7) {
        snprintf(text, sizeof(text), "LEVEL %d", trans->room_num);
        if (trans->open_transition || trans->close_transition) {
            draw_text_left(font, text, 142.0f, trans->text_y + 2.0f, 1.0f,
                           make_color_rgb(0, 128, 0));
            draw_text_left(font, text, 140.0f, trans->text_y, 1.0f, WHITE);
        }
    }
}

/* obj_bloom_appsrf Draw GUI Begin: threshold -> blur -> composite.
 * Each pass runs at the top level (raylib render targets do not nest). */
static void bloom_bright_pass(LongoRender *render)
{
    float threshold = 0.8f;
    float range = 0.3f;
    Rectangle flip = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       -(float)LONGO_LOGICAL_HEIGHT };
    Rectangle full = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       (float)LONGO_LOGICAL_HEIGHT };

    BeginTextureMode(render->bloom_ping);
    ClearBackground(BLANK);
    BeginShaderMode(render->bloom_lum_shader);
    SetShaderValue(render->bloom_lum_shader, render->lum_threshold_loc,
                   &threshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(render->bloom_lum_shader, render->lum_range_loc, &range,
                   SHADER_UNIFORM_FLOAT);
    DrawTexturePro(render->app_surface.texture, flip, full, (Vector2){ 0, 0 },
                   0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

static void bloom_blur_pass(LongoRender *render, RenderTexture2D *src,
                            RenderTexture2D *dst, float vec_x, float vec_y)
{
    float blur_steps = 5.0f; /* round(3.75) + 1 */
    float sigma = 0.2f;
    float texel[2] = { 1.0f / (float)LONGO_LOGICAL_WIDTH,
                       1.0f / (float)LONGO_LOGICAL_HEIGHT };
    float vec[2] = { vec_x, vec_y };
    Rectangle flip = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       -(float)LONGO_LOGICAL_HEIGHT };
    Rectangle full = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       (float)LONGO_LOGICAL_HEIGHT };

    BeginTextureMode(*dst);
    ClearBackground(BLANK);
    BeginShaderMode(render->blur_shader);
    SetShaderValue(render->blur_shader, render->blur_steps_loc, &blur_steps,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(render->blur_shader, render->blur_sigma_loc, &sigma,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(render->blur_shader, render->blur_vector_loc, vec,
                   SHADER_UNIFORM_VEC2);
    SetShaderValue(render->blur_shader, render->blur_texel_loc, texel,
                   SHADER_UNIFORM_VEC2);
    DrawTexturePro(src->texture, flip, full, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

static void bloom_composite(LongoRender *render)
{
    float intensity = 0.6f;
    Rectangle flip = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       -(float)LONGO_LOGICAL_HEIGHT };
    Rectangle full = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       (float)LONGO_LOGICAL_HEIGHT };

    BeginShaderMode(render->bloom_blend_shader);
    SetShaderValue(render->bloom_blend_shader, render->blend_intensity_loc,
                   &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(render->bloom_blend_shader,
                          render->blend_bloom_tex_loc,
                          render->bloom_ping.texture);
    DrawTexturePro(render->app_surface.texture, flip, full, (Vector2){ 0, 0 },
                   0.0f, WHITE);
    EndShaderMode();
}

/* --------------------------------------------------------------- */
/* Frame                                                              */
/* --------------------------------------------------------------- */

/* Entity draw slots: static layers + sim entities + presentation fx. */
enum {
    SLOT_BG = 0,
    SLOT_TILES_1,
    SLOT_TILES_3,
    SLOT_DOG,
    SLOT_BOX,
    SLOT_SINK,
    SLOT_APPLE,
    SLOT_SKULL,
    SLOT_HOLE,
    SLOT_BUTTON,
    SLOT_DOOR,
    SLOT_FLOWER,
    SLOT_FLY,
    SLOT_GOAL,
    SLOT_BARK,
    SLOT_SMOKE,
    SLOT_POPUP,
    SLOT_SHADOWS,
    SLOT_TITLE
};

typedef struct DrawItem {
    int depth;
    int order;
    int slot;
    int index;
} DrawItem;

static int draw_item_compare(const void *a, const void *b)
{
    const DrawItem *ia = (const DrawItem *)a;
    const DrawItem *ib = (const DrawItem *)b;
    if (ia->depth != ib->depth) return ib->depth - ia->depth;
    return ia->order - ib->order;
}

static void draw_item(LongoRender *render, const SimWorld *world,
                      const Pres *pres, const DrawItem *item)
{
    switch (item->slot) {
    case SLOT_BG: {
        /* background layer: sprTile tiled, visible in every room */
        if (render->sprite_loaded[LONGO_SPR_TILE]) {
            Texture2D tile = render->sprites[LONGO_SPR_TILE];
            int fw = tile.width;
            int fh = tile.height;
            for (int y = 0; y < LONGO_LOGICAL_HEIGHT; y += fh) {
                for (int x = 0; x < LONGO_LOGICAL_WIDTH; x += fw) {
                    DrawTexture(tile, x, y, WHITE);
                }
            }
        }
        break;
    }
    case SLOT_TILES_1:
    case SLOT_TILES_3: {
        const LongoRoomTileMap *tiles = tile_map_for(world->room);
        if (tiles != NULL) {
            draw_tile_layer(render, item->slot == SLOT_TILES_1
                                        ? &tiles->tiles_1
                                        : &tiles->tiles_3);
        }
        break;
    }
    case SLOT_DOG:
        if (world->dog.alive) draw_dog_world(render, world, pres);
        break;
    case SLOT_BOX: {
        const SimBox *box = &world->boxes[item->index];
        if (box->alive)
            draw_sprite_origin(render, LONGO_SPR_BOX, 0,
                               pres->box_x[item->index],
                               pres->box_y[item->index], 1.0f, 1.0f, 0.0f,
                               WHITE, 1.0f);
        break;
    }
    case SLOT_SINK: {
        const PresSink *s = &pres->sinks[item->index];
        if (s->alive)
            draw_sprite_origin(render, LONGO_SPR_BOX, 0, s->x, s->y, 1.0f,
                               1.0f, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_APPLE: {
        const SimItem *a = &world->apples[item->index];
        if (a->alive)
            draw_sprite_origin(render, LONGO_SPR_APPLE,
                               frame_of(pres->apple_clock),
                               (float)(sim_cell_x(a->cell) * SIM_CELL),
                               (float)(sim_cell_y(a->cell) * SIM_CELL), 1.0f,
                               1.0f, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_SKULL: {
        const SimItem *s = &world->skulls[item->index];
        if (s->alive)
            draw_sprite_origin(render, LONGO_SPR_SKULL,
                               frame_of(pres->pear_clock),
                               (float)(sim_cell_x(s->cell) * SIM_CELL),
                               (float)(sim_cell_y(s->cell) * SIM_CELL), 1.0f,
                               1.0f, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_HOLE: {
        const SimHole *h = &world->holes[item->index];
        if (h->alive)
            draw_sprite_origin(render, LONGO_SPR_HOLE, h->full ? 1 : 0,
                               (float)(sim_cell_x(h->cell) * SIM_CELL),
                               (float)(sim_cell_y(h->cell) * SIM_CELL), 1.0f,
                               1.0f, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_BUTTON: {
        const SimButton *b = &world->buttons[item->index];
        if (b->alive)
            draw_sprite_origin(render,
                               b->pressed ? LONGO_SPR_BUTTONPRESSED
                                          : LONGO_SPR_BUTTON,
                               frame_of(pres->button_clock),
                               (float)(sim_cell_x(b->zone[0]) * SIM_CELL),
                               (float)(sim_cell_y(b->zone[0]) * SIM_CELL),
                               1.0f, 1.0f, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_DOOR: {
        const SimDoor *d = &world->doors[item->index];
        const PresDoor *pd = &pres->doors[item->index];
        if (d->alive)
            draw_sprite_origin(render, LONGO_SPR_DOOR, 0, pd->x, pd->y,
                               pd->scale_x, pd->scale_y, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_FLOWER:
        draw_sprite_origin(render, LONGO_SPR_FLOWER,
                           frame_of(pres->flower_clock),
                           pres->flowers[item->index].x,
                           pres->flowers[item->index].y, 1.0f, 1.0f, 0.0f,
                           WHITE, 1.0f);
        break;
    case SLOT_FLY: {
        const PresButterfly *fly = &pres->flies[item->index];
        if (fly->alive)
            draw_sprite_origin(render, LONGO_SPR_FLY,
                               frame_of(pres->fly_clock), fly->x, fly->y,
                               1.0f, 1.0f, 0.0f, WHITE, 1.0f);
        break;
    }
    case SLOT_GOAL:
        if (world->goal.alive) draw_goal(render, world, pres);
        break;
    case SLOT_BARK: {
        const PresBark *b = &pres->barks[item->index];
        if (b->alive)
            draw_sprite_origin(render, LONGO_SPR_BARK, frame_of(b->frame),
                               b->x, b->y, 1.0f, 1.0f, b->angle, WHITE, 1.0f);
        break;
    }
    case SLOT_SMOKE: {
        const PresSmoke *s = &pres->smoke[item->index];
        if (s->alive) draw_smoke_puff(render, s);
        break;
    }
    case SLOT_POPUP: {
        const PresPopup *o = &pres->popups[item->index];
        if (o->alive)
            draw_sprite_origin(render, LONGO_SPR_ONE, o->variant, o->x, o->y,
                               1.0f, 1.0f, 0.0f, WHITE, o->alpha);
        break;
    }
    case SLOT_SHADOWS: {
        /* composited here in the depth order (oShadows depth 210) */
        if (world->dog.alive) {
            Rectangle src = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                              -(float)LONGO_LOGICAL_HEIGHT };
            Color tint = { 255, 255, 255, (unsigned char)(255 * 0.2f) };
            DrawTexturePro(render->shadow_surface.texture, src,
                           (Rectangle){ 0, 0, (float)LONGO_LOGICAL_WIDTH,
                                        (float)LONGO_LOGICAL_HEIGHT },
                           (Vector2){ 0, 0 }, 0.0f, tint);
        }
        break;
    }
    case SLOT_TITLE:
        if (pres->has_title_decor) draw_title(render, pres);
        break;
    default:
        break;
    }
}

void longo_render_frame(LongoRender *render, const SimWorld *world,
                        const Pres *pres)
{
    DrawItem items[20 + SIM_MAX_BOXES + SIM_MAX_HOLES + SIM_MAX_ITEMS * 2 +
                   SIM_MAX_BUTTONS + SIM_MAX_DOORS + PRES_MAX_FLOWERS +
                   PRES_MAX_FLIES + PRES_MAX_SMOKE + PRES_MAX_POPUPS +
                   PRES_MAX_BARKS + PRES_MAX_SINKS];
    int item_count = 0;
    const LongoRoomTileMap *tiles = tile_map_for(world->room);
    Rectangle flip = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       -(float)LONGO_LOGICAL_HEIGHT };
    Rectangle full = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       (float)LONGO_LOGICAL_HEIGHT };

    build_shadow_surface(render, world, pres);

    /* 1. application surface */
    BeginTextureMode(render->app_surface);
    ClearBackground(BLACK);

    /* depth-sorted drawables: background(700), tiles, shadow, entities */
    items[item_count++] = (DrawItem){ 700, 0, SLOT_BG, 0 };
    if (tiles != NULL) {
        items[item_count++] =
            (DrawItem){ tiles->tiles_1.depth, 1, SLOT_TILES_1, 0 };
        items[item_count++] =
            (DrawItem){ tiles->tiles_3.depth, 2, SLOT_TILES_3, 0 };
    }
    if (pres->shadows_present)
        items[item_count++] = (DrawItem){ 210, 3, SLOT_SHADOWS, 0 };
    if (world->dog.alive) items[item_count++] = (DrawItem){ 0, 4, SLOT_DOG, 0 };
    for (int i = 0; i < world->box_count; i++) {
        if (!world->boxes[i].alive) continue;
        items[item_count++] = (DrawItem){
            (int)(-100 - pres->box_y[i] / 6), 10 + i, SLOT_BOX, i
        };
    }
    for (int i = 0; i < pres->sink_count; i++)
        items[item_count++] = (DrawItem){ -100, 40 + i, SLOT_SINK, i };
    for (int i = 0; i < world->apple_count; i++)
        if (world->apples[i].alive)
            items[item_count++] = (DrawItem){ 100, 50 + i, SLOT_APPLE, i };
    for (int i = 0; i < world->skull_count; i++)
        if (world->skulls[i].alive)
            items[item_count++] = (DrawItem){ 100, 90 + i, SLOT_SKULL, i };
    for (int i = 0; i < world->hole_count; i++)
        if (world->holes[i].alive)
            items[item_count++] = (DrawItem){ 200, 130 + i, SLOT_HOLE, i };
    for (int i = 0; i < world->button_count; i++)
        if (world->buttons[i].alive)
            items[item_count++] = (DrawItem){ 200, 150 + i, SLOT_BUTTON, i };
    for (int i = 0; i < world->door_count; i++)
        if (world->doors[i].alive)
            items[item_count++] = (DrawItem){ 100, 170 + i, SLOT_DOOR, i };
    for (int i = 0; i < pres->flower_count; i++)
        items[item_count++] = (DrawItem){ 200, 180 + i, SLOT_FLOWER, i };
    for (int i = 0; i < PRES_MAX_FLIES; i++)
        if (pres->flies[i].alive)
            items[item_count++] = (DrawItem){ -500, 200 + i, SLOT_FLY, i };
    if (world->goal.alive)
        items[item_count++] = (DrawItem){ -180, 210, SLOT_GOAL, 0 };
    for (int i = 0; i < pres->bark_count; i++)
        if (pres->barks[i].alive)
            items[item_count++] = (DrawItem){ -500, 220 + i, SLOT_BARK, i };
    for (int i = 0; i < pres->smoke_count; i++)
        if (pres->smoke[i].alive)
            items[item_count++] = (DrawItem){ -1000, 240 + i, SLOT_SMOKE, i };
    for (int i = 0; i < pres->popup_count; i++)
        if (pres->popups[i].alive)
            items[item_count++] = (DrawItem){ -200000, 420 + i, SLOT_POPUP, i };
    if (pres->has_title_decor)
        items[item_count++] = (DrawItem){ -600, 440, SLOT_TITLE, 0 };

    qsort(items, (size_t)item_count, sizeof(DrawItem), draw_item_compare);

    for (int i = 0; i < item_count; i++) {
        draw_item(render, world, pres, &items[i]);
    }
    EndTextureMode();

    /* 2. bloom: obj_bloom_appsrf Draw GUI Begin (top-level passes) */
    bloom_bright_pass(render);
    bloom_blur_pass(render, &render->bloom_ping, &render->bloom_pong, 1.0f,
                    0.0f);
    bloom_blur_pass(render, &render->bloom_pong, &render->bloom_ping, 0.0f,
                    1.0f);

    /* 3. GUI surface: bloom composite then Draw GUI events */
    BeginTextureMode(render->gui_surface);
    ClearBackground(BLACK);
    bloom_composite(render);
    draw_tutorial_gui(render, world, pres);
    draw_transition_gui(render, world);
    EndTextureMode();

    /* 4. present: GUI surface fills the window (GUI space scales 4x) */
    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle screen = { 0.0f, 0.0f, (float)GetScreenWidth(),
                         (float)GetScreenHeight() };
    DrawTexturePro(render->gui_surface.texture, flip, screen, (Vector2){ 0, 0 },
                   0.0f, WHITE);
    EndDrawing();
}

void longo_render_dispatch_sounds(LongoRender *render, SimWorld *world)
{
    SimSoundEvent events[SIM_MAX_SOUNDS];
    int count = sim_poll_sounds(world, events);
    for (int i = 0; i < count; i++) {
        int snd = events[i].sound;
        if (snd < 0 || snd > 6) continue;
        if (events[i].loop) {
            if (render->music_loaded && !render->music_playing) {
                PlaySound(render->music);
                render->music_playing = true;
            }
            continue;
        }
        if (render->sound_loaded[snd]) PlaySound(render->sounds[snd]);
    }
}
