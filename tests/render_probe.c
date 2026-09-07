/*
 * Hidden-window render probe (manual diagnostic, not a ctest).
 *
 * Replays synthesized view streams through the full raylib backend and
 * reads the presented pixels back, asserting the present-pass
 * composition contracts on a real GPU:
 *   (i)   GUI coverage — the GUI canvas accumulates straight coverage
 *         (separate alpha blend factors), so a half-transparent red rect
 *         composites over blue as ~(128, 0, 127), not the (128, 0, 191)
 *         of coverage squared by ordinary alpha blending;
 *   (ii)  shipped sprites — the binary-alpha dialogue nine-patch
 *         composites exactly as straight-alpha "over" predicts (texel
 *         where opaque, backdrop where clear), the contract that lets
 *         the premultiplied accumulation replace ordinary blending;
 *   (iii) text depth — window-scale text behind a lower-depth rect in
 *         the same layer stays behind it.
 *
 * Needs a GPU/window: run the executable manually from the repo root
 * (the asset root and fonts resolve relative to the working directory).
 * Exit 0 = every contract held; 1 = a contract failed.
 */
#include "raylib.h"
#include "rlgl.h"

#include "render.h"
#include "core/world.h"
#include "core/view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static unsigned char *frame;
static int frame_w, frame_h;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) printf("PASS  %s\n", #cond);                            \
        else {                                                            \
            failures++;                                                   \
            printf("FAIL  %s (render_probe.c:%d)\n", #cond, __LINE__);    \
        }                                                                 \
    } while (0)

static void sample(int x, int y, unsigned char out[4])
{
    int i = (y * frame_w + x) * 4;
    out[0] = frame[i];
    out[1] = frame[i + 1];
    out[2] = frame[i + 2];
    out[3] = frame[i + 3];
}

static int within(unsigned char c, int want, int tol)
{
    return abs((int)c - want) <= tol;
}

static int count_colors(int x0, int y0, int x1, int y1, int r, int g, int b)
{
    int hits = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            unsigned char c[4];
            sample(x, y, c);
            if (c[0] == r && c[1] == g && c[2] == b) hits++;
        }
    return hits;
}

/* Present the current stream twice (drawing never advances it) and read
 * the back buffer: after the second swap GL_BACK holds the first,
 * identical present. */
static void capture(LongoRender *render, const SimWorld *world)
{
    free(frame);
    longo_render_frame(render, world);
    longo_render_frame(render, world);
    frame_w = GetScreenWidth();
    frame_h = GetScreenHeight();
    frame = rlReadScreenPixels(frame_w, frame_h);
}

/* One full-screen blue world base (the surface the GUI canvas composites
 * over), at a depth below every GUI scenario item. */
static void push_blue_world(void)
{
    view_layer(VIEW_WORLD);
    view_rect(1, 0.0f, 0.0f, (float)LONGO_LOGICAL_WIDTH,
              (float)LONGO_LOGICAL_HEIGHT, view_rgb(0, 0, 255));
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(LONGO_LOGICAL_WIDTH * LONGO_WINDOW_SCALE,
               LONGO_LOGICAL_HEIGHT * LONGO_WINDOW_SCALE, "render-probe");

    LongoRender render;
    memset(&render, 0, sizeof(render));
    if (!longo_render_init(&render, NULL)) {
        printf("render_probe: asset initialization failed; run from the "
               "repo root\n");
        CloseWindow();
        return 2;
    }
    sim_init(1u);

    /* (i) GUI coverage: red alpha 128 (depth 0, front) over blue (depth
     * 1, back), straight onto the world's blue base. */
    view_begin_frame();
    push_blue_world();
    view_layer(VIEW_GUI);
    view_rect(1, 100.0f, 100.0f, 104.0f, 104.0f, view_rgb(0, 0, 255));
    {
        ViewColor red128 = { 255, 0, 0, 128 };
        view_rect(0, 110.0f, 110.0f, 84.0f, 84.0f, red128);
    }
    view_sort();
    capture(&render, world_ptr());
    {
        unsigned char c[4];
        sample(600, 600, c); /* logical (150, 150): red over blue */
        printf("(i) red alpha 128 over blue = (%d, %d, %d)\n", c[0], c[1],
               c[2]);
        CHECK(within(c[0], 128, 1) && c[1] == 0 && within(c[2], 127, 1));
    }

    /* (ii) shipped sprite: the dialogue nine-patch, drawn at integer
     * geometry so a corner cell maps 1:1 onto the logical grid.  Every
     * opaque texel must land exactly, every clear texel must leave the
     * backdrop untouched. */
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/sprites/sprDialogueBox/"
                                     "sprDialogueBox_0.png",
                 render.asset_root);
        Image img = LoadImage(path);
        CHECK(img.data != NULL);
        if (img.data != NULL) {
            int cell = img.width / 3 > img.height / 3 ? img.height / 3
                                                      : img.width / 3;
            int binary = 1, mismatches = 0;
            view_begin_frame();
            push_blue_world();
            view_layer(VIEW_GUI);
            view_nine_patch(1, LONGO_SPR_DIALOGUEBOX, 0, 100.0f, 100.0f,
                            60.0f, 60.0f, view_rgb(255, 255, 255), 1.0f);
            view_sort();
            capture(&render, world_ptr());
            for (int ty = 0; ty < cell && binary; ty++)
                for (int tx = 0; tx < cell; tx++) {
                    Color texel = GetImageColor(img, tx, ty);
                    unsigned char got[4];
                    /* window pixels sit at 4x the logical grid; sample
                     * one pixel inside the texel's footprint */
                    sample(400 + tx * 4 + 1, 400 + ty * 4 + 1, got);
                    if (texel.a != 0 && texel.a != 255) binary = 0;
                    if (texel.a == 255
                            ? (got[0] != texel.r || got[1] != texel.g ||
                               got[2] != texel.b)
                            : (got[0] != 0 || got[1] != 0 || got[2] != 255))
                        mismatches++;
                }
            CHECK(binary); /* the shipped sprite really is binary alpha */
            CHECK(mismatches == 0);
            if (!binary)
                printf("(ii) sprite has partial alpha; premultiplied "
                       "accumulation is no longer pixel-equal for it\n");
        }
        UnloadImage(img);
    }

    /* (iii) text depth: text at depth 2 (back) must hide behind a rect
     * at depth 0 (front) in both layers, while an uncovered control
     * text still renders. */
    view_begin_frame();
    push_blue_world();
    view_layer(VIEW_WORLD);
    view_text(2, 1, "TESTING DEPTH", 152.0f, 104.0f, 1.0f,
              view_rgb(255, 0, 255));
    view_rect(0, 60.0f, 92.0f, 184.0f, 24.0f, view_rgb(0, 200, 0));
    view_layer(VIEW_GUI);
    view_text(2, 1, "TESTING DEPTH", 152.0f, 104.0f, 1.0f,
              view_rgb(255, 0, 255));
    view_rect(0, 60.0f, 92.0f, 184.0f, 24.0f, view_rgb(0, 200, 0));
    view_text(0, 1, "CONTROL", 152.0f, 40.0f, 1.0f, view_rgb(255, 0, 255));
    view_sort();
    capture(&render, world_ptr());
    {
        /* the rect: logical x 60..244, y 92..116 at the 4x window scale */
        int covered = count_colors(240, 368, 976, 464, 255, 0, 255);
        int control = count_colors(400, 140, 800, 210, 255, 0, 255);
        printf("(iii) text px inside the front rect: %d; control text px: "
               "%d\n",
               covered, control);
        CHECK(covered == 0);
        CHECK(control > 0);
    }

    free(frame);
    longo_render_shutdown(&render);
    CloseWindow();
    printf("render_probe: %s (%d failure(s))\n",
           failures ? "FAILED" : "all contracts held", failures);
    return failures ? 1 : 0;
}
