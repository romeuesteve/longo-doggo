/*
 * Longo Doggo render backend.
 *
 * Pure replay layer: object scripts push view items through core/view.h;
 * this module owns the raylib assets and surfaces and replays the sorted
 * item lists:
 *   - application surface at the 304x208 logical resolution
 *   - global.shadow_surf rebuilt per frame, composited at 0.2 alpha
 *   - GUI surface: transparent canvas, refilled per run; the present
 *     pass composites world -> window-scale text between the gfx runs
 *     of each layer (segment compositing), so the stream's depth order
 *     is the composition order — within a layer too
 *   - static tile layers (sprTile ground, room Tiles_3) stamped into
 *     cached surfaces per room and composited as one quad per layer
 *
 * Render targets never nest: every surface is filled top-level or from
 * the default framebuffer, so the per-room tile cache is rebuilt before
 * the surface passes begin.
 *
 * Resource lifecycle: longo_render_init() resolves the asset roots and
 * acquires every resource — render targets, sprite strips, the tile
 * atlas, fonts and sounds — and the LongoRender struct owns them for
 * its whole lifetime; longo_render_shutdown() releases exactly what
 * init acquired.  Required assets (targets, sprites, tile atlas, digits
 * font) fail init loudly after releasing the partial acquisition; audio
 * is optional and degrades to silence with a warning.
 */
#include "render.h"
#include "room_tiles.h"
#include "sprites.h"

/* blend-factor plumbing for the GUI canvas accumulation (rlgl) */
#include "rlgl.h"

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

/* The UI font is repo-local (assets/fonts), not part of exported-assets:
 * the packaged layout ships it beside the asset root, a repo checkout
 * keeps it under assets/fonts.  Resolved once here so the lazy font
 * loader never roots around on its own. */
static void resolve_ui_font_path(LongoRender *render)
{
    snprintf(render->ui_font_path, sizeof(render->ui_font_path),
             "fonts/renogare.ttf");
    make_asset_path(render, render->ui_font_path, render->ui_font_path,
                    sizeof(render->ui_font_path));
    if (!file_exists(render->ui_font_path))
        snprintf(render->ui_font_path, sizeof(render->ui_font_path),
                 "assets/fonts/renogare.ttf");
}

/* One 304x208 target with point filtering: every surface pass needs the
 * same kind of canvas (app, gui, shadow and the two cached tile
 * layers). */
static RenderTexture2D load_logical_target(void)
{
    RenderTexture2D target =
        LoadRenderTexture(LONGO_LOGICAL_WIDTH, LONGO_LOGICAL_HEIGHT);
    if (target.id != 0)
        SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    return target;
}

/* Load every animation frame of one sprite into a horizontal strip so a
 * frame index maps to a source rectangle.  Sprite frames are required
 * assets: a missing file or frame returns failure and init aborts,
 * naming the sprite — a silent skip here drew blank slots forever. */
static bool load_sprite(LongoRender *render, LongoSprite sprite)
{
    const char *name = longo_sprite_name(sprite);
    int frames = longo_sprite_frames(sprite);
    char path[1024];
    Image first;
    Image strip;
    Texture2D tex;
    int fw, fh;
    bool ok = false;

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
        if (!file_exists(fpath)) goto done; /* required frame missing */
        frame_img = LoadImage(fpath);
        if (frame_img.data == NULL) goto done;
        ImageDraw(&strip, frame_img,
                  (Rectangle){ 0, 0, (float)frame_img.width,
                               (float)frame_img.height },
                  (Rectangle){ (float)(i * fw), 0, (float)frame_img.width,
                               (float)frame_img.height },
                  WHITE);
        UnloadImage(frame_img);
    }
    tex = LoadTextureFromImage(strip);
    if (tex.id != 0) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        render->sprites[sprite] = tex;
        render->sprite_loaded[sprite] = true;
        ok = true;
    }
done:
    UnloadImage(strip);
    return ok;
}

/* The pixelated digits font, used only for the in-world house counter
 * (font_id 2), which always draws a plain "%d".  The ten digit cells are
 * the exported glyphs_FontDigits.csv metrics (5x13 glyphs, fixed 6px
 * advance); no CSV parsing needed.  Required: without it the house
 * counter would silently vanish. */
static bool load_digits_font(LongoRender *render)
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
    if (!file_exists(path)) return false;
    font->texture = LoadTexture(path);
    if (font->texture.id == 0) return false;
    SetTextureFilter(font->texture, TEXTURE_FILTER_POINT);
    for (int i = 0; i < 10; i++) {
        font->glyph[i].source = digits[i].source;
        font->glyph[i].offset = digits[i].offset;
    }
    font->loaded = true;
    return true;
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

/* Audio is optional: without a device (or a file) the game runs silent
 * instead of failing, so only a warning marks the gap. */
static Sound load_sound_relative(const LongoRender *render, const char *file,
                                 bool *loaded)
{
    char path[1024];
    snprintf(path, sizeof(path), "audio/audiogroup_default/%s", file);
    make_asset_path(render, path, path, sizeof(path));
    if (!render->audio_ready) {
        if (loaded) *loaded = false;
        return (Sound){ 0 };
    }
    if (!file_exists(path)) {
        TraceLog(LOG_WARNING, "render: audio file missing: %s", path);
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

    /* the one place asset roots are resolved: packaged
     * (assets/exported-assets), local and requested layouts all land
     * here, and every loader below reads the resolved paths */
    select_asset_root(render, asset_root);
    resolve_ui_font_path(render);

    /* every surface pass needs all five targets */
    render->app_surface = load_logical_target();
    render->gui_surface = load_logical_target();
    render->shadow_surface = load_logical_target();
    render->tile_cache.ground = load_logical_target();
    render->tile_cache.decor = load_logical_target();
    if (render->app_surface.id == 0 || render->gui_surface.id == 0 ||
        render->shadow_surface.id == 0 ||
        render->tile_cache.ground.id == 0 ||
        render->tile_cache.decor.id == 0) {
        TraceLog(LOG_ERROR, "render: render target allocation failed");
        longo_render_shutdown(render);
        return false;
    }

    /* sprites: required, every frame of every entry in the sprite
     * table (entries without a name are unused slots) */
    for (int s = 1; s < 32; s++) {
        if (longo_sprite_name(s) == NULL) continue;
        if (!load_sprite(render, (LongoSprite)s)) {
            TraceLog(LOG_ERROR, "render: missing sprite frames for %s",
                     longo_sprite_name(s));
            longo_render_shutdown(render);
            return false;
        }
    }

    /* tile atlas: required (the Tiles_3 decoration layer draws from it) */
    {
        char path[1024];
        snprintf(path, sizeof(path), "backgrounds/TileSet1.png");
        make_asset_path(render, path, path, sizeof(path));
        if (file_exists(path)) render->tileset1 = LoadTexture(path);
        render->tileset1_loaded = render->tileset1.id != 0;
        if (!render->tileset1_loaded) {
            TraceLog(LOG_ERROR, "render: missing or unreadable %s", path);
            longo_render_shutdown(render);
            return false;
        }
        SetTextureFilter(render->tileset1, TEXTURE_FILTER_POINT);
    }

    /* pixel digits font: required (the house counter draws from it) */
    if (!load_digits_font(render)) {
        TraceLog(LOG_ERROR, "render: missing fonts/FontDigits.png");
        longo_render_shutdown(render);
        return false;
    }

    /* the UI font stays lazy (first text draw) and optional: its
     * GetFontDefault fallback keeps the game readable without the
     * repo-local Renogare file */
    if (!IsAudioDeviceReady()) InitAudioDevice();
    render->audio_ready = IsAudioDeviceReady();
    if (!render->audio_ready)
        TraceLog(LOG_WARNING,
                 "render: no audio device; sounds are disabled");
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
    return true;
}

void longo_render_shutdown(LongoRender *render)
{
    if (render == NULL) return;
    /* every release is guarded by the flags init set, so this is safe
     * on a partially initialised struct (the init failure paths call
     * it) and idempotent on an already-shut-down one */
    for (int s = 0; s < 32; s++) {
        if (render->sprite_loaded[s]) UnloadTexture(render->sprites[s]);
    }
    if (render->tileset1_loaded) UnloadTexture(render->tileset1);
    if (render->font_digits.loaded) UnloadTexture(render->font_digits.texture);
    if (render->ui_font_owned) UnloadFont(render->ui_font);
    for (int s = 0; s < 7; s++) {
        if (render->sound_loaded[s]) UnloadSound(render->sounds[s]);
    }
    if (render->music_loaded) UnloadSound(render->music);
    if (render->app_surface.id != 0)
        UnloadRenderTexture(render->app_surface);
    if (render->gui_surface.id != 0)
        UnloadRenderTexture(render->gui_surface);
    if (render->shadow_surface.id != 0)
        UnloadRenderTexture(render->shadow_surface);
    if (render->tile_cache.ground.id != 0)
        UnloadRenderTexture(render->tile_cache.ground);
    if (render->tile_cache.decor.id != 0)
        UnloadRenderTexture(render->tile_cache.decor);
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

/* The UI font loads on first use from the path init resolved
 * (ui_font_path); it lives in the struct, so shutdown unloads it.  When
 * the file is absent the default font keeps the game readable — the
 * substitution is optional, the game is not. */
static Font text_font(LongoRender *render)
{
    if (!render->ui_font_ready) {
        render->ui_font_ready = true;
        if (file_exists(render->ui_font_path))
            render->ui_font = LoadFontEx(render->ui_font_path,
                                         (int)LONGO_TEXT_BASE_PX, NULL, 0);
        render->ui_font_owned = render->ui_font.texture.id != 0;
        if (render->ui_font_owned) {
            /* the atlas is baked at exactly the drawn pixel size, so
             * glyphs draw without resampling */
            SetTextureFilter(render->ui_font.texture,
                             TEXTURE_FILTER_POINT);
        } else {
            TraceLog(LOG_WARNING,
                     "render: UI font not found (%s); using the default "
                     "font", render->ui_font_path);
            render->ui_font = GetFontDefault();
        }
    }
    return render->ui_font;
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
                      Color color, int *starts, int *lens, int cap)
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
            const char *line_start = cursor;
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
            if (starts != NULL && lens != NULL && count < cap) {
                size_t begin = (size_t)(line_start - line);
                size_t end_span = (size_t)(cursor - line);
                while (end_span > begin && line[end_span - 1] == ' ')
                    end_span--;
                starts[count] = (int)((p - text) + begin);
                lens[count] = (int)(end_span - begin);
            }
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

/* Dialogue pages stay on screen for many frames while the text is
 * unchanged, but the wrap is O(words^2) MeasureTextEx probes, so the
 * measured line segments are cached (render->wrap_cache) and reused
 * until the text or layout changes. */
static bool wrap_cache_matches(const LongoRender *render, Font font,
                               const char *text, float width, float size)
{
    const struct LongoWrapCache *cache = &render->wrap_cache;
    return cache->valid &&
           cache->font.texture.id == font.texture.id &&
           cache->width == width && cache->size == size &&
           strcmp(cache->text, text) == 0;
}

static void draw_text_wrapped(LongoRender *render, Font font,
                              const char *text, float x, float y, float sep,
                              float width, float size, Color color)
{
    struct LongoWrapCache *cache = &render->wrap_cache;
    if (!wrap_cache_matches(render, font, text, width, size)) {
        cache->valid = false;
        cache->count = wrap_lines(font, text, x, 0.0f, sep, width, size,
                                  false, color, cache->starts, cache->lens,
                                  LONGO_WRAP_CACHE_LINES);
        if (cache->count > 0 && cache->count <= LONGO_WRAP_CACHE_LINES) {
            snprintf(cache->text, sizeof(cache->text), "%s", text);
            cache->font = font;
            cache->width = width;
            cache->size = size;
            cache->valid = true;
        }
    }
    int count = cache->count;
    if (count <= 0) return;
    float height = (float)(count - 1) * sep + size;
    float line_y = y - height * 0.5f;
    for (int i = 0; i < count; i++) {
        char line[512];
        size_t len = (size_t)cache->lens[i];
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, text + cache->starts[i], len);
        line[len] = '\0';
        draw_text_line(font, line, x, line_y, size, true, color);
        line_y += sep;
    }
}

/* Is this a present-pass replay item: UI text drawn straight to the
 * window at window scale?  (The pixel digits font is low-res surface
 * content and stays with the layer's other gfx items.) */
static bool is_window_text(const ViewItem *it)
{
    return (it->kind == VIEW_ITEM_TEXT ||
            it->kind == VIEW_ITEM_TEXT_WRAPPED) && it->font_id != 2;
}

/* Present-pass replay of one layer's UI text item, in the item's sorted
 * composition position (the segment walk in present_layer_segments
 * places it between the gfx runs the stream puts around it).  The atlas
 * is baked at the drawn pixel size (logical 10px * the 4x window scale)
 * with point filtering, so text is crisp at any window size. */
static void draw_text_item_window_scale(LongoRender *render,
                                        const ViewItem *it)
{
    Font font = text_font(render);
    float rx = text_ratio_x();
    float ry = text_ratio_y();
    /* the atlas is baked at exactly this size for the default window
     * scale, so glyphs draw without resampling */
    float size = LONGO_TEXT_BASE_PX * it->xscale;
    bool centered = it->font_id != 0;
    if (it->kind == VIEW_ITEM_TEXT) {
        draw_text_line(font, it->text, it->x * rx, it->y * ry, size,
                       centered, to_ray_color(it->color));
    } else {
        draw_text_wrapped(render, font, it->text, it->x * rx,
                          it->y * ry, it->line_sep * it->xscale * ry,
                          it->text_width * rx, size,
                          to_ray_color(it->color));
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
/* Static tile layers, pre-rendered per room                          */
/* --------------------------------------------------------------- */
static void draw_ground_tiles(LongoRender *render)
{
    if (!render->sprite_loaded[LONGO_SPR_TILE]) return;
    Texture2D tile = render->sprites[LONGO_SPR_TILE];
    for (int y = 0; y < LONGO_LOGICAL_HEIGHT; y += tile.height)
        for (int x = 0; x < LONGO_LOGICAL_WIDTH; x += tile.width)
            DrawTexture(tile, x, y, WHITE);
}

static void tile_cache_ensure(LongoRender *render,
                              const LongoRoomTileMap *tiles)
{
    if (render->tile_cache.decor_for != tiles) {
        BeginTextureMode(render->tile_cache.decor);
        ClearBackground(BLANK);
        if (tiles != NULL) draw_tile_layer(render, &tiles->tiles_3);
        EndTextureMode();
        render->tile_cache.decor_for = tiles;
    }
    if (!render->tile_cache.ground_valid) {
        BeginTextureMode(render->tile_cache.ground);
        ClearBackground(BLACK);
        draw_ground_tiles(render);
        EndTextureMode();
        render->tile_cache.ground_valid = true;
    }
}

/* --------------------------------------------------------------- */
/* View replay                                                        */
/* --------------------------------------------------------------- */

static void replay_item(LongoRender *render, const ViewItem *it)
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
        /* font 0/1 text replays in the present pass at window scale, in
         * its layer's sorted composition position (see
         * draw_text_items_window_scale) */
        break;
    case VIEW_ITEM_TEXT_WRAPPED:
        /* replayed in the present pass at window scale (see
         * draw_text_items_window_scale); skip here */
        break;
    case VIEW_ITEM_SHADOW_COMPOSITE:
        /* world_draw() emits the item only when the composite belongs
         * in the frame (the room has shadows and the dog is alive); the
         * replay consumes the stream, it never queries gameplay state */
        {
            Color tint = { 255, 255, 255, (unsigned char)(255 * 0.2f) };
            DrawTexturePro(render->shadow_surface.texture, rect_flip(),
                           rect_full(), (Vector2){ 0, 0 }, 0.0f, tint);
        }
        break;
    case VIEW_ITEM_TILE_LAYERS: {
        /* tile_cache_ensure() ran top-level before the surface passes;
         * render targets never nest */
        const RenderTexture2D *surf = it->frame == 0
                                          ? &render->tile_cache.ground
                                          : &render->tile_cache.decor;
        DrawTexturePro(surf->texture, rect_flip(), rect_full(),
                       (Vector2){ 0, 0 }, 0.0f, WHITE);
        break;
    }
    default:
        break;
    }
}

static void replay_layer(LongoRender *render, ViewLayer layer)
{
    int count;
    const ViewItem *items = view_items(layer, &count);
    for (int i = 0; i < count; i++)
        replay_item(render, &items[view_order_at(layer, i)]);
}

/* --------------------------------------------------------------- */
/* Present pass: run/text segments                                    */
/* --------------------------------------------------------------- */

/* Fill the GUI canvas with one maximal run of a layer's non-text items
 * and composite it over the present pass.  Inside the target the items
 * accumulate with separate blend factors (RGB: SRC_ALPHA /
 * ONE_MINUS_SRC_ALPHA, alpha: ONE / ONE_MINUS_SRC_ALPHA), so RGB stacks
 * premultiplied while alpha stacks coverage and the canvas ends up
 * holding (rgb * a, a) — exactly what the premultiplied blit below
 * needs to blend every pixel exactly once.  Ordinary alpha blending
 * would square the coverage: a red alpha-128 rect composites as
 * (128, 0, 191) over blue instead of (128, 0, 127). */
static void composite_layer_run(LongoRender *render, ViewLayer layer,
                                int from, int to, Rectangle screen)
{
    int count;
    const ViewItem *items = view_items(layer, &count);

    BeginTextureMode(render->gui_surface);
    ClearBackground(BLANK);
    /* factors feed BLEND_CUSTOM_SEPARATE; EndBlendMode restores the
     * normal mode (the factors stay latched but inert until the mode is
     * entered again, which always re-sets them) */
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE,
                              RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD,
                              RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    for (int i = from; i < to; i++)
        replay_item(render, &items[view_order_at(layer, i)]);
    EndBlendMode();
    EndTextureMode();

    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawTexturePro(render->gui_surface.texture, rect_flip(), screen,
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndBlendMode();
}

/* Present one layer as the segments the sorted stream describes: every
 * maximal run of non-text items is composited through the GUI canvas
 * (cleared per run, so each pixel blends exactly once) and the text
 * items that follow it draw at window scale, in depth order — a text
 * item behind a lower-depth rect stays behind it, and text between two
 * gfx runs lands between them.
 *
 * The world layer's backmost run rides in the opaque application
 * surface (base_composited: blitted by the caller), so only runs past
 * the first text item need the canvas again; a layer without text never
 * touches it, which keeps the common frame at one fill per surface. */
static void present_layer_segments(LongoRender *render, ViewLayer layer,
                                   Rectangle screen, bool base_composited)
{
    int count;
    const ViewItem *items = view_items(layer, &count);
    bool base_pending = base_composited;
    bool text_drawn = false;
    int i = 0;

    while (i < count) {
        int run_end = i;
        while (run_end < count &&
               !is_window_text(&items[view_order_at(layer, run_end)]))
            run_end++;
        if (run_end > i) {
            if (base_pending && !text_drawn)
                base_pending = false; /* backmost run: in the app surface */
            else
                composite_layer_run(render, layer, i, run_end, screen);
        }
        while (run_end < count &&
               is_window_text(&items[view_order_at(layer, run_end)])) {
            draw_text_item_window_scale(render,
                                        &items[view_order_at(layer, run_end)]);
            text_drawn = true;
            run_end++;
        }
        i = run_end;
    }
}

void longo_render_frame(LongoRender *render, const SimWorld *world)
{
    /* rebuild the static tile surfaces on room change, outside any
     * render-target pass */
    tile_cache_ensure(render, room_tiles_for(world->room));

    view_sort();

    /* shadow surface from the shadow layer */
    BeginTextureMode(render->shadow_surface);
    ClearBackground(BLANK);
    replay_layer(render, VIEW_SHADOW);
    EndTextureMode();

    /* application surface from the world layer */
    BeginTextureMode(render->app_surface);
    ClearBackground(BLACK);
    replay_layer(render, VIEW_WORLD);
    EndTextureMode();

    /* present, in the composition order the sorted stream describes:
     * the world composite, then per layer, window-scale text between
     * the gfx runs that surround it (segment compositing, see
     * present_layer_segments).  Text still rides at window resolution
     * (crisp at any window size instead of resampled with the pixel
     * surface), and gfx runs re-composited after it keep the stream's
     * depth order within the layer too. */
    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle screen = { 0.0f, 0.0f, (float)GetScreenWidth(),
                         (float)GetScreenHeight() };
    DrawTexturePro(render->app_surface.texture, rect_flip(), screen,
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
    present_layer_segments(render, VIEW_WORLD, screen, true);
    present_layer_segments(render, VIEW_GUI, screen, false);
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
            render->music_requested = true;
            continue;
        }
        if (render->sound_loaded[snd]) PlaySound(render->sounds[snd]);
    }
    /* raylib Sounds play exactly one pass, so the looping track is
     * restarted whenever it is not playing: that carries it through
     * room transitions and past a suspended browser audio context */
    if (render->music_requested && render->music_loaded &&
        !IsSoundPlaying(render->music))
        PlaySound(render->music);
}
