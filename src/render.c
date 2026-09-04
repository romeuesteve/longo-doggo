/*
 * Longo Doggo render backend.
 *
 * Pure replay layer: object scripts push view items through core/view.h;
 * this module owns the raylib assets and surfaces and replays the sorted
 * item lists:
 *   - application surface at 304x208 (the original surface_resize size)
 *   - global.shadow_surf rebuilt per frame, composited at 0.2 alpha
 *   - GUI surface: obj_bloom_appsrf bloom composite, then GUI items
 *
 * Render targets never nest: the shadow surface, application surface,
 * bloom ping-pong passes and GUI surface are all filled top-level.
 */
#include "render.h"
#include "room_tiles.h"
#include "sprites.h"

#include "objects/dog.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * GameMaker frame index maps to a source rectangle. */
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
    render->sounds[SND_BARK] = load_sound_relative(render, "snd_bark.wav",
        &render->sound_loaded[SND_BARK]);
    render->sounds[SND_BUTTON] = load_sound_relative(render,
        "snd_button.wav", &render->sound_loaded[SND_BUTTON]);
    render->sounds[SND_POOF] = load_sound_relative(render, "snd_poof.wav",
        &render->sound_loaded[SND_POOF]);
    render->sounds[SND_PUSHED] = load_sound_relative(render,
        "snd_pushed.wav", &render->sound_loaded[SND_PUSHED]);
    render->sounds[SND_WIN] = load_sound_relative(render, "snd_win.wav",
        &render->sound_loaded[SND_WIN]);
    render->sounds[SND_WRONG] = load_sound_relative(render,
        "snd_wrong.wav", &render->sound_loaded[SND_WRONG]);
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
/* Replay primitives                                                 */
/* --------------------------------------------------------------- */

static Color to_ray_color(ViewColor c)
{
    return (Color){ c.r, c.g, c.b, c.a };
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
                    out[0] != '\0')
                    break;
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
/* Tile layers (8x8 tiles addressed in 12x12 atlas cells)             */
/* --------------------------------------------------------------- */

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

/* --------------------------------------------------------------- */
/* View replay                                                        */
/* --------------------------------------------------------------- */

static void replay_item(LongoRender *render, const SimWorld *world,
                        const ViewItem *it)
{
    switch (it->kind) {
    case VIEW_ITEM_SPRITE:
        draw_sprite_origin(render, it->sprite, it->frame, it->x, it->y,
                           it->xscale, it->yscale, it->rotation,
                           to_ray_color(it->color), it->alpha);
        break;
    case VIEW_ITEM_SPRITE_PART:
        draw_sprite_part_ext(render, it->sprite, it->frame, (int)it->radius,
                             (int)it->rotation, (int)it->w, (int)it->h,
                             it->x, it->y, it->xscale, it->yscale,
                             to_ray_color(it->color), it->alpha);
        break;
    case VIEW_ITEM_LINE:
        draw_line_width_color((Vector2){ it->x, it->y },
                              (Vector2){ it->x2, it->y2 }, it->radius,
                              to_ray_color(it->color),
                              to_ray_color(it->color2));
        break;
    case VIEW_ITEM_CIRCLE:
        if (it->color.r == it->color2.r && it->color.g == it->color2.g &&
            it->color.b == it->color2.b)
            DrawCircleV((Vector2){ it->x, it->y }, it->radius,
                        to_ray_color(it->color));
        else
            DrawCircleGradient((Vector2){ it->x, it->y }, it->radius,
                               to_ray_color(it->color),
                               to_ray_color(it->color2));
        break;
    case VIEW_ITEM_RECT:
        DrawRectangle((int)it->x, (int)it->y, (int)it->w, (int)it->h,
                      to_ray_color(it->color));
        break;
    case VIEW_ITEM_TEXT:
        draw_text_left(font_by_asset(render, it->font_id), it->text, it->x,
                       it->y, it->xscale, to_ray_color(it->color));
        break;
    case VIEW_ITEM_TEXT_WRAPPED:
        draw_text_ext_centered(font_by_asset(render, it->font_id), it->text,
                               it->x, it->y, it->line_sep, it->text_width,
                               it->xscale, to_ray_color(it->color));
        break;
    case VIEW_ITEM_SHADOW_COMPOSITE:
        if (dog_alive()) {
            Rectangle src = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                              -(float)LONGO_LOGICAL_HEIGHT };
            Color tint = { 255, 255, 255, (unsigned char)(255 * 0.2f) };
            DrawTexturePro(render->shadow_surface.texture, src,
                           (Rectangle){ 0, 0, (float)LONGO_LOGICAL_WIDTH,
                                        (float)LONGO_LOGICAL_HEIGHT },
                           (Vector2){ 0, 0 }, 0.0f, tint);
        }
        break;
    case VIEW_ITEM_TILE_LAYERS: {
        const LongoRoomTileMap *tiles = room_tiles_for(world->room);
        if (it->frame == 0) {
            if (render->sprite_loaded[LONGO_SPR_TILE]) {
                Texture2D tile = render->sprites[LONGO_SPR_TILE];
                for (int y = 0; y < LONGO_LOGICAL_HEIGHT; y += tile.height)
                    for (int x = 0; x < LONGO_LOGICAL_WIDTH; x += tile.width)
                        DrawTexture(tile, x, y, WHITE);
            }
        } else if (it->frame == 1 && tiles != NULL) {
            draw_tile_layer(render, &tiles->tiles_1);
        } else if (it->frame == 2 && tiles != NULL) {
            draw_tile_layer(render, &tiles->tiles_3);
        }
        break;
    }
    default:
        break;
    }
}

static void replay_layer(LongoRender *render, const SimWorld *world,
                         ViewLayer layer)
{
    int count;
    const ViewItem *items = view_items(layer, &count);
    for (int i = 0; i < count; i++)
        replay_item(render, world, &items[i]);
}

/* obj_bloom_appsrf Draw GUI Begin: threshold -> blur -> composite. */
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
    float blur_steps = 5.0f;
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

void longo_render_frame(LongoRender *render, const SimWorld *world)
{
    Rectangle flip = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       -(float)LONGO_LOGICAL_HEIGHT };
    Rectangle full = { 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                       (float)LONGO_LOGICAL_HEIGHT };

    view_sort();

    /* shadow surface from the shadow layer */
    BeginTextureMode(render->shadow_surface);
    ClearBackground(BLANK);
    replay_layer(render, world, VIEW_SHADOW);
    EndTextureMode();

    /* application surface from the world layer */
    BeginTextureMode(render->app_surface);
    ClearBackground(BLACK);
    replay_layer(render, world, VIEW_WORLD);
    EndTextureMode();

    /* bloom */
    bloom_bright_pass(render);
    bloom_blur_pass(render, &render->bloom_ping, &render->bloom_pong, 1.0f,
                    0.0f);
    bloom_blur_pass(render, &render->bloom_pong, &render->bloom_ping, 0.0f,
                    1.0f);

    /* GUI surface: bloom composite then GUI items */
    BeginTextureMode(render->gui_surface);
    ClearBackground(BLACK);
    bloom_composite(render);
    replay_layer(render, world, VIEW_GUI);
    EndTextureMode();

    /* present */
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
    SoundEvent ev[EVENTS_MAX_SOUNDS];
    int count = events_poll_sounds(ev);
    for (int i = 0; i < count; i++) {
        int snd = ev[i].sound;
        if (snd <= SND_NONE || snd > SND_BARK) continue;
        if (ev[i].loop) {
            if (render->music_loaded && !render->music_playing) {
                PlaySound(render->music);
                render->music_playing = true;
            }
            continue;
        }
        if (render->sound_loaded[snd]) PlaySound(render->sounds[snd]);
    }
}
