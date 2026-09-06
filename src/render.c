/*
 * Longo Doggo render backend.
 *
 * Pure replay layer: object scripts push view items through core/view.h;
 * this module owns the raylib assets and surfaces and replays the sorted
 * item lists:
 *   - application surface at the 304x208 logical resolution
 *   - global.shadow_surf rebuilt per frame, composited at 0.2 alpha
 *   - GUI surface: the application surface, then GUI items
 *
 * Render targets never nest: the shadow surface, application surface
 * and GUI surface are all filled top-level.
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
        "../assets/exported-assets",
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

/* Load every animation frame of one sprite into a horizontal strip so a
 * frame index maps to a source rectangle. */
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

/* The pixelated digits font, used only for the in-world house counter
 * (font_id 2), which always draws a plain "%d".  The ten digit cells are
 * the exported glyphs_FontDigits.csv metrics (5x13 glyphs, fixed 6px
 * advance); no CSV parsing needed. */
static void load_digits_font(LongoRender *render)
{
    static const struct {
        Rectangle source;
        unsigned char offset;
    } digits[10] = {
        { { 108, 32, 5, 13 }, 0 }, { { 102, 32, 4, 13 }, 1 },
        { {  95, 32, 5, 13 }, 0 }, { {  88, 32, 5, 13 }, 0 },
        { {  81, 32, 5, 13 }, 0 }, { {  11, 47, 4, 13 }, 1 },
        { {  83, 47, 5, 13 }, 0 }, { {  90, 47, 5, 13 }, 0 },
        { {  97, 47, 5, 13 }, 0 }, { { 113, 62, 5, 13 }, 0 },
    };
    LongoDigitsFont *font = &render->font_digits;
    char path[1024];

    snprintf(path, sizeof(path), "fonts/FontDigits.png");
    make_asset_path(render, path, path, sizeof(path));
    if (!file_exists(path)) return;
    font->texture = LoadTexture(path);
    if (font->texture.id == 0) return;
    SetTextureFilter(font->texture, TEXTURE_FILTER_POINT);
    for (int i = 0; i < 10; i++) {
        font->glyph[i].source = digits[i].source;
        font->glyph[i].offset = digits[i].offset;
    }
    font->loaded = true;
}

/* Centered on (x, y): the counter text is always a "%d" number. */
static void draw_digits_text(const LongoDigitsFont *font, const char *text,
                             float x, float y, Color color)
{
    if (font == NULL || !font->loaded || text == NULL) return;
    int len = (int)strlen(text);
    x -= (float)len * 3.0f; /* fixed 6px advance */
    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c < '0' || c > '9') {
            x += 6.0f;
            continue;
        }
        const Rectangle *src = &font->glyph[c - '0'].source;
        Rectangle dest = { x, y + (float)font->glyph[c - '0'].offset,
                           src->width, src->height };
        DrawTexturePro(font->texture, *src, dest, (Vector2){ 0, 0 }, 0.0f,
                       color);
        x += 6.0f;
    }
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
    SetTextureFilter(render->app_surface.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(render->gui_surface.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(render->shadow_surface.texture, TEXTURE_FILTER_POINT);

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
    }

    load_digits_font(render);

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
    if (render->tileset1_loaded) UnloadTexture(render->tileset1);
    if (render->font_digits.loaded) UnloadTexture(render->font_digits.texture);
    for (int s = 0; s < 7; s++) {
        if (render->sound_loaded[s]) UnloadSound(render->sounds[s]);
    }
    if (render->music_loaded) UnloadSound(render->music);
    UnloadRenderTexture(render->app_surface);
    UnloadRenderTexture(render->gui_surface);
    UnloadRenderTexture(render->shadow_surface);
    memset(render, 0, sizeof(*render));
}

/* --------------------------------------------------------------- */
/* Replay primitives                                                 */
/* --------------------------------------------------------------- */

static Color to_ray_color(ViewColor c)
{
    return (Color){ c.r, c.g, c.b, c.a };
}

/* draw_sprite_origin(): (x, y) is the sprite origin position.  Sprite
 * row 0 maps to y - oy*yscale and row fh to y + (fh - oy)*yscale,
 * mirroring the texture when a scale is negative — a negative yscale on
 * a top-left origin extends the sprite upward.  raylib only flips via
 * negative source rects, so derive the on-screen AABB, flip the source,
 * and pivot the rotation at the origin point. */
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

    float x0 = x - (float)ox * xscale;
    float x1 = x + (float)(fw - ox) * xscale;
    float y0 = y - (float)oy * yscale;
    float y1 = y + (float)(fh - oy) * yscale;
    Rectangle source = { (float)(frame * fw), 0.0f, (float)fw, (float)fh };
    if (xscale < 0.0f) {
        source.x += source.width;
        source.width = -source.width;
    }
    if (yscale < 0.0f) {
        source.y += source.height;
        source.height = -source.height;
    }
    Rectangle dest = { x, y, fabsf(x1 - x0), fabsf(y1 - y0) };
    /* raylib's dest.x/y is where the origin point lands (the quad spans
     * dest - origin .. dest + size - origin), so dest stays on the
     * sprite's origin point and origin carries the offset to the AABB
     * corner */
    Vector2 origin = { x - fminf(x0, x1), y - fminf(y0, y1) };
    Color c = tint;
    c.a = (unsigned char)(255.0f * alpha + 0.5f);
    DrawTexturePro(render->sprites[sprite], source, dest, origin, -rotation,
                   c);
}

/* draw_sprite_part_ext(): source region in frame coordinates, clipped to
 * the frame bounds; no origin offset. */
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

/* 9-slice panel: the corners keep their native size, the edges and the
 * centre cell stretch to the destination rect (the dialogue panel). */
static void draw_nine_patch(LongoRender *render, LongoSprite sprite,
                            int frame, float x, float y, float w, float h,
                            Color tint, float alpha)
{
    if (sprite <= LONGO_SPR_NONE || !render->sprite_loaded[sprite]) return;
    int frames = longo_sprite_frames(sprite);
    if (frames <= 0) return;
    int fw = render->sprites[sprite].width / frames;
    int fh = render->sprites[sprite].height;
    if (frame < 0) frame = 0;
    if (frame >= frames) frame %= frames;
    float tex_x = (float)(frame * fw);
    float cx = fw / 3.0f, cy = fh / 3.0f;
    float iw = w - 2.0f * cx;
    float ih = h - 2.0f * cy;
    if (iw < 0.0f) iw = 0.0f;
    if (ih < 0.0f) ih = 0.0f;
    Color c = tint;
    c.a = (unsigned char)(255.0f * alpha + 0.5f);
    Texture2D tex = render->sprites[sprite];

    struct {
        Rectangle src, dst;
    } parts[9] = {
        { { tex_x + 0.0f, 0.0f, cx, cy }, { x, y, cx, cy } },
        { { tex_x + cx, 0.0f, cx, cy }, { x + cx, y, iw, cy } },
        { { tex_x + 2 * cx, 0.0f, cx, cy }, { x + cx + iw, y, cx, cy } },
        { { tex_x + 0.0f, cy, cx, cy }, { x, y + cy, cx, ih } },
        { { tex_x + cx, cy, cx, cy }, { x + cx, y + cy, iw, ih } },
        { { tex_x + 2 * cx, cy, cx, cy }, { x + cx + iw, y + cy, cx, ih } },
        { { tex_x + 0.0f, 2 * cy, cx, cy }, { x, y + cy + ih, cx, cy } },
        { { tex_x + cx, 2 * cy, cx, cy }, { x + cx, y + cy + ih, iw, cy } },
        { { tex_x + 2 * cx, 2 * cy, cx, cy },
          { x + cx + iw, y + cy + ih, cx, cy } }
    };
    for (int i = 0; i < 9; i++)
        DrawTexturePro(tex, parts[i].src, parts[i].dst, (Vector2){ 0, 0 },
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
/* Text: Renogare at window resolution; pixel digits in-world         */
/* --------------------------------------------------------------- */

/* UI text renders with Renogare (repo assets/fonts; converted to
 * TrueType outlines so stb_truetype can rasterize it).  The atlas is
 * baked at the drawn pixel size (logical 10px * the 4x window scale)
 * with point filtering, so text is crisp inside the low-res surface.
 * Alignment: font 0 (bold) left-aligned, fonts 1/2 centered. */

#define LONGO_TEXT_BASE_PX (10.0f * (float)LONGO_WINDOW_SCALE)

static Font text_font(LongoRender *render)
{
    static Font ui_font;
    static bool initialised;
    if (!initialised) {
        initialised = true;
        char path[1024];
        snprintf(path, sizeof(path), "fonts/renogare.ttf");
        make_asset_path(render, path, path, sizeof(path));
        if (!file_exists(path)) {
            snprintf(path, sizeof(path), "assets/fonts/renogare.ttf");
        }
        ui_font = file_exists(path)
                      ? LoadFontEx(path, (int)LONGO_TEXT_BASE_PX, NULL, 0)
                      : GetFontDefault();
        if (ui_font.texture.id != 0 && ui_font.glyphCount > 0)
            SetTextureFilter(ui_font.texture, TEXTURE_FILTER_POINT);
    }
    return ui_font;
}

static float text_ratio_y(void)
{
    return (float)GetScreenHeight() / (float)LONGO_LOGICAL_HEIGHT;
}

static float text_ratio_x(void)
{
    return (float)GetScreenWidth() / (float)LONGO_LOGICAL_WIDTH;
}

static void draw_text_line(Font font, const char *text, float x, float y,
                           float size, bool centered, Color color)
{
    if (text == NULL || text[0] == '\0') return;
    if (centered) {
        Vector2 m = MeasureTextEx(font, text, size, 0.0f);
        x -= m.x * 0.5f;
    }
    DrawTextEx(font, text, (Vector2){ x, y }, size, 0.0f, color);
}

/* draw_text_ext_transformed() with fa_center: word wrap at `width`,
 * line separation `sep`, at window scale.  Handles embedded newlines.
 * Pass draw=false to only count the wrapped lines. */
static int wrap_lines(Font font, const char *text, float x, float y,
                      float sep, float width, float size, bool draw,
                      Color color)
{
    if (text == NULL) return 0;
    const char *p = text;
    float line_y = y;
    int count = 0;
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
                if (MeasureTextEx(font, probe, size, 0.0f).x > width &&
                    out[0] != '\0')
                    break;
                snprintf(out, sizeof(out), "%s", probe);
                cursor = w_end;
            }
            if (draw) draw_text_line(font, out, x, line_y, size, true, color);
            line_y += sep;
            count++;
            if (*cursor == ' ') cursor++;
        }
        p += len;
        if (nl != NULL) p++;
    }
    return count;
}

/* Centered on (x, y) both ways: the block is offset up by half its
 * height so multi-line text stays inside the bubble. */
static void draw_text_wrapped(Font font, const char *text, float x, float y,
                              float sep, float width, float size, Color color)
{
    int count = wrap_lines(font, text, x, 0.0f, sep, width, size, false,
                           color);
    if (count <= 0) return;
    float height = (float)(count - 1) * sep + size;
    wrap_lines(font, text, x, y - height * 0.5f, sep, width, size, true,
               color);
}

/* Present-pass replay of the UI text items (everything except the
 * pixel digits font), in push order per layer. */
static void draw_text_items_window_scale(LongoRender *render)
{
    Font font = text_font(render);
    float rx = text_ratio_x();
    float ry = text_ratio_y();
    for (int layer = VIEW_WORLD; layer <= VIEW_GUI; layer++) {
        int count;
        const ViewItem *items = view_items((ViewLayer)layer, &count);
        for (int i = 0; i < count; i++) {
            const ViewItem *it = &items[i];
            if (it->kind != VIEW_ITEM_TEXT &&
                it->kind != VIEW_ITEM_TEXT_WRAPPED)
                continue;
            if (it->font_id == 2) continue; /* in-world digits font */
            /* the atlas is baked at exactly this size for the default
             * window scale, so glyphs draw without resampling */
            float size = LONGO_TEXT_BASE_PX * it->xscale;
            bool centered = it->font_id != 0;
            if (it->kind == VIEW_ITEM_TEXT) {
                draw_text_line(font, it->text, it->x * rx, it->y * ry, size,
                               centered, to_ray_color(it->color));
            } else {
                draw_text_wrapped(font, it->text, it->x * rx, it->y * ry,
                                  it->line_sep * it->xscale * ry,
                                  it->text_width * rx, size,
                                  to_ray_color(it->color));
            }
        }
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
    atlas = render->tileset1;
    if (!render->tileset1_loaded) return;
    columns = 8;
    tile_count = 64;
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

/* Full-surface rectangles used by every surface-to-surface blit; the
 * negative height flips the render target's bottom-up orientation. */
static Rectangle rect_flip(void)
{
    return (Rectangle){ 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                        -(float)LONGO_LOGICAL_HEIGHT };
}

static Rectangle rect_full(void)
{
    return (Rectangle){ 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
                        (float)LONGO_LOGICAL_HEIGHT };
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
        draw_sprite_part_ext(render, it->sprite, it->frame, (int)it->src_x,
                             (int)it->src_y, (int)it->w, (int)it->h,
                             it->x, it->y, it->xscale, it->yscale,
                             to_ray_color(it->color), it->alpha);
        break;
    case VIEW_ITEM_NINE_PATCH:
        draw_nine_patch(render, it->sprite, it->frame, it->x, it->y, it->w,
                        it->h, to_ray_color(it->color), it->alpha);
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
        if (it->font_id == 2) {
            /* the house counter keeps the pixel digits font and lives
             * inside the low-res surface with everything else */
            draw_digits_text(&render->font_digits, it->text, it->x, it->y,
                             to_ray_color(it->color));
        }
        break;
    case VIEW_ITEM_TEXT_WRAPPED:
        /* replayed in the present pass at window scale (see
         * draw_text_items_window_scale); skip here */
        break;
    case VIEW_ITEM_SHADOW_COMPOSITE:
        if (dog_alive()) {
            Color tint = { 255, 255, 255, (unsigned char)(255 * 0.2f) };
            DrawTexturePro(render->shadow_surface.texture, rect_flip(),
                           rect_full(), (Vector2){ 0, 0 }, 0.0f, tint);
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

void longo_render_frame(LongoRender *render, const SimWorld *world)
{
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

    /* GUI surface: the application surface, then GUI items */
    BeginTextureMode(render->gui_surface);
    ClearBackground(BLACK);
    DrawTexturePro(render->app_surface.texture, rect_flip(), rect_full(),
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
    replay_layer(render, world, VIEW_GUI);
    EndTextureMode();

    /* present */
    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle screen = { 0.0f, 0.0f, (float)GetScreenWidth(),
                         (float)GetScreenHeight() };
    DrawTexturePro(render->gui_surface.texture, rect_flip(), screen,
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
    /* text rides on top at window resolution: crisp at any window size
     * instead of resampled with the pixel surface */
    draw_text_items_window_scale(render);
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
