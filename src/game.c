/*
 * Longo Doggo - GameMaker-faithful simulation core.
 *
 * Every event below is a direct port of the recovered decompiled GML in
 * recovered/exported-assets/code (same variable names, same constants, same
 * execution order).  GameMaker semantics reproduced here:
 *   - instance ids: creation order, room placements first
 *   - depth-sorted drawing is the renderer's job
 *   - bbox collision with strict overlap (flush edges do not collide)
 *   - alarms count down once per frame and fire on reaching zero
 *   - instances created mid-frame skip Step until the next frame
 */
#include "game.h"

#include <math.h>
#include <string.h>

#define LONGO_PI 3.14159265358979f

/* ------------------------------------------------------------------ */
/* Sprite metadata recovered from data.win (see analysis/ground-truth) */
/* ------------------------------------------------------------------ */
typedef struct LongoSpriteInfo {
    int width, height;
    int origin_x, origin_y;
    int frames;
    int fps; /* GameMaker sprite-editor playback speed (FramesPerSecond);
                the engine advances image_index by image_speed * fps / 60 */
} LongoSpriteInfo;

static const LongoSpriteInfo LONGO_SPRITES[] = {
    [LONGO_SPR_BLOCK] = { 16, 16, 0, 0, 2 , 0 },
    [LONGO_SPR_HOLE] = { 16, 16, 0, 0, 9 , 0 },
    [LONGO_SPR_BUTTON] = { 16, 16, 0, 0, 9 , 8 },
    [LONGO_SPR_TRANSITION] = { 64, 64, 0, 0, 2 , 30 },
    [LONGO_SPR_DOGPAW] = { 16, 16, 8, 8, 1 , 8 },
    [LONGO_SPR_OLDDOG] = { 64, 64, 0, 0, 5 , 8 },
    [LONGO_SPR_HOUSE] = { 64, 64, 32, 64, 2 , 0 },
    [LONGO_SPR_FLY] = { 16, 16, 0, 0, 8 , 16 },
    [LONGO_SPR_DOGUP] = { 16, 16, 8, 8, 4 , 8 },
    [LONGO_SPR_BUTTON43] = { 32, 32, 0, 0, 9 , 8 },
    [LONGO_SPR_PEAR] = { 16, 16, 0, 0, 8 , 16 },
    [LONGO_SPR_BOX] = { 16, 20, 0, 4, 1 , 30 },
    [LONGO_SPR_ICON] = { 16, 16, 0, 0, 1 , 0 },
    [LONGO_SPR_BARK] = { 16, 16, 8, 8, 7 , 18 },
    [LONGO_SPR_TILE] = { 32, 32, 0, 0, 1 , 30 },
    [LONGO_SPR_FLOWER] = { 8, 8, 0, 0, 4 , 8 },
    [LONGO_SPR_SMOKE] = { 16, 16, 8, 8, 1 , 30 },
    [LONGO_SPR_DOGTAIL] = { 16, 16, 8, 8, 4 , 8 },
    [LONGO_SPR_TITLE] = { 191, 149, 0, 0, 3 , 30 },
    [LONGO_SPR_DOGLEFT] = { 16, 16, 8, 8, 4 , 8 },
    [LONGO_SPR_DOGRIGHT] = { 16, 16, 8, 8, 4 , 8 },
    [LONGO_SPR_DIALOGUEBOX] = { 24, 24, 12, 12, 1 , 30 },
    [LONGO_SPR_APPLE] = { 16, 16, 0, 0, 8 , 16 },
    [LONGO_SPR_SKULL] = { 18, 18, 0, 0, 4 , 8 },
    [LONGO_SPR_ICON2] = { 16, 16, 0, 0, 1 , 30 },
    [LONGO_SPR_BUTTONPRESSED] = { 16, 16, 0, 0, 1 , 8 },
    [LONGO_SPR_ONE] = { 8, 8, 0, 0, 2 , 0 },
    [LONGO_SPR_DOOR] = { 16, 16, 0, 0, 1 , 0 },
    [LONGO_SPR_DOGDOWN] = { 16, 16, 8, 8, 4 , 8 },
};

static LongoSprite obj_default_sprite(int object)
{
    switch (object) {
    case LONGO_BLOOM: return LONGO_SPR_TRANSITION; /* spr_placeholder_module unused */
    case LONGO_OBJ_SKULL: return LONGO_SPR_PEAR;
    case LONGO_OBJ_TUTORIAL: return LONGO_SPR_DIALOGUEBOX;
    case LONGO_OBJ_BUTTERFLY: return LONGO_SPR_FLY;
    case LONGO_OBJ_BLOCK: return LONGO_SPR_BLOCK;
    case LONGO_OBJ_BUTTON: return LONGO_SPR_BUTTON;
    case LONGO_OBJ_HOUSESPAWNER: return LONGO_SPR_ICON2;
    case LONGO_OBJ_APPLE: return LONGO_SPR_APPLE;
    case LONGO_OBJ_STUPIDBLOCK: return LONGO_SPR_BLOCK;
    case LONGO_OBJ_TRANSITION: return LONGO_SPR_NONE;
    case LONGO_OBJ_HOLE: return LONGO_SPR_HOLE;
    case LONGO_OBJ_MOUSE: return LONGO_SPR_DOGPAW;
    case LONGO_OBJ_TITLE: return LONGO_SPR_TITLE;
    case LONGO_OBJ_GOALUP: return LONGO_SPR_NONE;
    case LONGO_OBJ_WIN: return LONGO_SPR_BLOCK;
    case LONGO_OBJ_GOAL: return LONGO_SPR_HOUSE;
    case LONGO_OBJ_FLOWER: return LONGO_SPR_FLOWER;
    case LONGO_OBJ_DOOR: return LONGO_SPR_DOOR;
    case LONGO_OBJ_DOGPART: return LONGO_SPR_DOGPAW;
    case LONGO_OBJ_SMOKE: return LONGO_SPR_SMOKE;
    case LONGO_OBJ_BARK: return LONGO_SPR_BARK;
    case LONGO_OBJ_DOGSPAWNER: return LONGO_SPR_ICON;
    case LONGO_OBJ_BOX: return LONGO_SPR_BOX;
    case LONGO_OBJ_SHADOWS: return LONGO_SPR_NONE;
    case LONGO_OBJ_DOG: return LONGO_SPR_DOGUP;
    case LONGO_OBJ_ONE: return LONGO_SPR_ONE;
    default: return LONGO_SPR_NONE;
    }
}

static int obj_default_visible(int object)
{
    switch (object) {
    case LONGO_OBJ_BLOCK:
    case LONGO_OBJ_STUPIDBLOCK:
    case LONGO_OBJ_WIN:
    case LONGO_OBJ_DOGPART:
        return 0;
    default:
        return 1;
    }
}

/* oBlock child classes: collision checks against oBlock include them. */
int longo_is_block_family(int object)
{
    switch (object) {
    case LONGO_OBJ_BLOCK:
    case LONGO_OBJ_HOLE:
    case LONGO_OBJ_DOOR:
    case LONGO_OBJ_GOAL:
    case LONGO_OBJ_BOX:
    case LONGO_OBJ_DOGPART:
        return 1;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/* GameMaker math                                                      */
/* ------------------------------------------------------------------ */
static float g_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static float g_clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float g_sign(float v)
{
    if (v > 0) return 1.0f;
    if (v < 0) return -1.0f;
    return 0.0f;
}

static float g_lengthdir_x(float len, float dir)
{
    return cosf(dir * (LONGO_PI / 180.0f)) * len;
}

static float g_lengthdir_y(float len, float dir)
{
    return -sinf(dir * (LONGO_PI / 180.0f)) * len;
}

static float g_point_direction(float x1, float y1, float x2, float y2)
{
    float dir = atan2f(-(y2 - y1), x2 - x1) * (180.0f / LONGO_PI);
    if (dir < 0) dir += 360.0f;
    return dir;
}

/* Wave() global script, verbatim. */
float longo_wave(float a, float b, float period, float phase, double time_ms)
{
    float a4 = (b - a) * 0.5f;
    float t = (float)(time_ms * 0.001);
    return a + a4 + sinf(((t + period * phase) / period) * (2.0f * LONGO_PI)) * a4;
}

int longo_sprite_width(int sprite)
{
    if (sprite <= LONGO_SPR_NONE || sprite >= (int)(sizeof(LONGO_SPRITES) / sizeof(LONGO_SPRITES[0])))
        return 0;
    return LONGO_SPRITES[sprite].width;
}

int longo_sprite_height(int sprite)
{
    if (sprite <= LONGO_SPR_NONE || sprite >= (int)(sizeof(LONGO_SPRITES) / sizeof(LONGO_SPRITES[0])))
        return 0;
    return LONGO_SPRITES[sprite].height;
}

int longo_sprite_origin_x(int sprite)
{
    if (sprite <= LONGO_SPR_NONE || sprite >= (int)(sizeof(LONGO_SPRITES) / sizeof(LONGO_SPRITES[0])))
        return 0;
    return LONGO_SPRITES[sprite].origin_x;
}

int longo_sprite_origin_y(int sprite)
{
    if (sprite <= LONGO_SPR_NONE || sprite >= (int)(sizeof(LONGO_SPRITES) / sizeof(LONGO_SPRITES[0])))
        return 0;
    return LONGO_SPRITES[sprite].origin_y;
}

int longo_sprite_frames(int sprite)
{
    if (sprite <= LONGO_SPR_NONE || sprite >= (int)(sizeof(LONGO_SPRITES) / sizeof(LONGO_SPRITES[0])))
        return 0;
    return LONGO_SPRITES[sprite].frames;
}

static unsigned int rng_next(LongoWorld *w)
{
    /* xorshift32; GameMaker's exact generator is not recoverable and only
     * affects cosmetic scatter (smoke drift, butterfly wander, dog barks). */
    unsigned int x = w->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    w->rng = x ? x : 0x9e3779b9u;
    return w->rng;
}

static float g_random(LongoWorld *w, float max)
{
    return (float)(rng_next(w) & 0xFFFFFF) / (float)0x1000000 * max;
}

static float g_random_range(LongoWorld *w, float lo, float hi)
{
    return lo + (float)(rng_next(w) & 0xFFFFFF) / (float)0x1000000 * (hi - lo);
}

/* ------------------------------------------------------------------ */
/* Sound helpers                                                       */
/* ------------------------------------------------------------------ */
static void play_sound(LongoWorld *w, int sound, int loop)
{
    if (w->sound_count < LONGO_MAX_SOUNDS) {
        w->sounds[w->sound_count].sound = sound;
        w->sounds[w->sound_count].loop = loop;
        w->sound_count++;
    }
}

/* ------------------------------------------------------------------ */
/* Instance access                                                     */
/* ------------------------------------------------------------------ */
LongoInst *longo_find_id(const LongoWorld *w, int id)
{
    if (id < 0 || id >= w->instance_count) return NULL;
    LongoInst *inst = &((LongoWorld *)w)->instances[id];
    return inst->alive ? inst : NULL;
}

static int class_matches(int object, int class_obj)
{
    if (class_obj == LONGO_OBJ_BLOCK) return longo_is_block_family(object);
    return object == class_obj;
}

LongoInst *longo_find_first(const LongoWorld *w, int object)
{
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &((LongoWorld *)w)->instances[i];
        if (inst->alive && class_matches(inst->object, object)) return inst;
    }
    return NULL;
}

int longo_instance_exists(const LongoWorld *w, int object)
{
    return longo_find_first(w, object) != NULL;
}

int longo_instance_number(const LongoWorld *w, int object)
{
    int n = 0;
    for (int i = 0; i < w->instance_count; i++) {
        const LongoInst *inst = &w->instances[i];
        if (inst->alive && class_matches(inst->object, object)) n++;
    }
    return n;
}

static void create_event(LongoWorld *w, LongoInst *inst);

static LongoInst *instance_create(LongoWorld *w, float x, float y, int object,
                                  int depth, float xscale, float yscale)
{
    if (w->instance_count >= LONGO_MAX_INSTANCES) return NULL;
    LongoInst *inst = &w->instances[w->instance_count];
    memset(inst, 0, sizeof(*inst));
    inst->id = w->instance_count++;
    inst->object = object;
    inst->alive = 1;
    inst->born_tick = w->current_tick;
    inst->x = x;
    inst->y = y;
    inst->xstart = x;
    inst->ystart = y;
    inst->xprev = x;
    inst->yprev = y;
    inst->depth = depth;
    inst->sprite_index = obj_default_sprite(object);
    inst->image_speed = 1.0f;
    inst->image_xscale = 1.0f;
    inst->image_yscale = 1.0f;
    inst->image_alpha = 1.0f;
    inst->visible = obj_default_visible(object);
    inst->alarm0 = -1;
    inst->alarm1 = -1;
    inst->follow = -4; /* GameMaker noone */
    inst->dir = 0;
    inst->image_xscale = xscale;
    inst->image_yscale = yscale;
    w->next_id = w->instance_count;
    create_event(w, inst);
    return inst;
}

static void instance_destroy(LongoInst *inst)
{
    inst->alive = 0;
}

/* ------------------------------------------------------------------ */
/* Collision (bbox, strict overlap like GameMaker)                     */
/* ------------------------------------------------------------------ */
static void inst_bbox(const LongoInst *inst, float x, float y, float *bx1,
                      float *by1, float *bx2, float *by2)
{
    const LongoSpriteInfo *spr = &LONGO_SPRITES[inst->sprite_index];
    *bx1 = x - (float)spr->origin_x * inst->image_xscale;
    *by1 = y - (float)spr->origin_y * inst->image_yscale;
    *bx2 = *bx1 + (float)spr->width * inst->image_xscale;
    *by2 = *by1 + (float)spr->height * inst->image_yscale;
}

static int rects_overlap(float ax1, float ay1, float ax2, float ay2,
                         float bx1, float by1, float bx2, float by2)
{
    /* GameMaker's bbox test: touching edges are NOT a collision. */
    return ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1;
}

static int inst_meets_at(const LongoWorld *w, const LongoInst *self, float x,
                         float y, int class_obj, LongoInst **first_hit)
{
    float ax1, ay1, ax2, ay2;
    inst_bbox(self, x, y, &ax1, &ay1, &ax2, &ay2);
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *other = &((LongoWorld *)w)->instances[i];
        float bx1, by1, bx2, by2;
        if (!other->alive || other == self) continue;
        if (!class_matches(other->object, class_obj)) continue;
        if (other->sprite_index == LONGO_SPR_NONE) continue;
        inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
        if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2)) {
            if (first_hit) *first_hit = other;
            return 1;
        }
    }
    return 0;
}

/* place_meeting() */
static int place_meeting(const LongoWorld *w, const LongoInst *self, float x,
                         float y, int class_obj)
{
    return inst_meets_at(w, self, x, y, class_obj, NULL);
}

/* instance_place() */
static LongoInst *instance_place(const LongoWorld *w, const LongoInst *self,
                                 float x, float y, int class_obj)
{
    LongoInst *hit = NULL;
    inst_meets_at(w, self, x, y, class_obj, &hit);
    return hit; /* NULL == noone */
}

/* collision_rectangle(x1,y1,x2,y2,class,prec,notme) */
static LongoInst *collision_rectangle(const LongoWorld *w, float x1, float y1,
                                      float x2, float y2, int class_obj)
{
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *other = &((LongoWorld *)w)->instances[i];
        float bx1, by1, bx2, by2;
        if (!other->alive) continue;
        if (!class_matches(other->object, class_obj)) continue;
        if (other->sprite_index == LONGO_SPR_NONE) continue;
        inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
        if (rects_overlap(x1, y1, x2, y2, bx1, by1, bx2, by2)) return other;
    }
    return NULL;
}

/* collision_line(x1,y1,x2,y2,class,prec,notme) against instance bboxes. */
static int seg_aabb(float x1, float y1, float x2, float y2,
                    float bx1, float by1, float bx2, float by2)
{
    float t0 = 0.0f, t1 = 1.0f;
    float dx = x2 - x1, dy = y2 - y1;
    float p[4] = { -dx, dx, -dy, dy };
    float q[4] = { x1 - bx1, bx2 - x1, y1 - by1, by2 - y1 };
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return 0;
        } else {
            float r = q[i] / p[i];
            if (p[i] < 0.0f) {
                if (r > t1) return 0;
                if (r > t0) t0 = r;
            } else {
                if (r < t0) return 0;
                if (r < t1) t1 = r;
            }
        }
    }
    return 1;
}

static int collision_line(const LongoWorld *w, float x1, float y1, float x2,
                          float y2, int class_obj)
{
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *other = &((LongoWorld *)w)->instances[i];
        float bx1, by1, bx2, by2;
        if (!other->alive) continue;
        if (!class_matches(other->object, class_obj)) continue;
        if (other->sprite_index == LONGO_SPR_NONE) continue;
        inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
        if (seg_aabb(x1, y1, x2, y2, bx1, by1, bx2, by2)) return 1;
    }
    return 0;
}

/* distance between two instances' bboxes (GameMaker distance_to_object) */
static float inst_distance(const LongoInst *a, const LongoInst *b)
{
    float ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
    float dx = 0.0f, dy = 0.0f;
    if (a->sprite_index == LONGO_SPR_NONE || b->sprite_index == LONGO_SPR_NONE)
        return hypotf(b->x - a->x, b->y - a->y);
    inst_bbox(a, a->x, a->y, &ax1, &ay1, &ax2, &ay2);
    inst_bbox(b, b->x, b->y, &bx1, &by1, &bx2, &by2);
    if (ax1 > bx2) dx = ax1 - bx2;
    else if (bx1 > ax2) dx = bx1 - ax2;
    if (ay1 > by2) dy = ay1 - by2;
    else if (by1 > ay2) dy = by1 - ay2;
    return hypotf(dx, dy);
}

static float inst_distance_to_point(const LongoInst *inst, float px, float py)
{
    float bx1, by1, bx2, by2;
    float dx = 0.0f, dy = 0.0f;
    if (inst->sprite_index == LONGO_SPR_NONE)
        return hypotf(px - inst->x, py - inst->y);
    inst_bbox(inst, inst->x, inst->y, &bx1, &by1, &bx2, &by2);
    if (px < bx1) dx = bx1 - px;
    else if (px > bx2) dx = px - bx2;
    if (py < by1) dy = by1 - py;
    else if (py > by2) dy = py - by2;
    return hypotf(dx, dy);
}

/* ------------------------------------------------------------------ */
/* Create events                                                       */
/* ------------------------------------------------------------------ */
static void create_event(LongoWorld *w, LongoInst *inst)
{
    switch (inst->object) {
    case LONGO_OBJ_APPLE:
    case LONGO_OBJ_SKULL:
        inst->depth = 100;
        break;

    case LONGO_OBJ_BLOCK:
        inst->block = 1;
        inst->push = 0;
        if (longo_instance_exists(w, LONGO_OBJ_MOUSE)) inst->visible = 1;
        break;

    case LONGO_OBJ_BOX:
        inst->xx = inst->x;
        inst->yy = inst->y;
        inst->block = 1;
        inst->push = 1;
        break;

    case LONGO_OBJ_BUTTON:
        inst->pressed = 0;
        inst->wait = 1;
        inst->sprite = 2; /* sprButton */
        inst->alarm0 = 5;
        inst->depth = 200;
        break;

    case LONGO_OBJ_BUTTERFLY:
        inst->vspd = g_clamp(0, -0.1f, 0.1f);
        inst->hspd = g_clamp(0, -0.1f, 0.1f);
        inst->dir = 0;
        inst->alarm0 = 10;
        inst->depth = -500;
        break;

    case LONGO_OBJ_DOGPART:
        inst->xx = inst->x;
        inst->yy = inst->y;
        inst->first = 0;
        inst->xprev = inst->xx;
        inst->yprev = inst->yy + 16;
        inst->draw_legs = 0;
        inst->block = 1;
        inst->push = 0;
        inst->legs_angle = 0;
        break;

    case LONGO_OBJ_DOG: {
        inst->dir = 180;
        inst->play = 1;
        inst->lengthstart = 5;
        inst->length = 5;
        inst->xx = inst->x;
        inst->yy = inst->y;
        inst->key_cooldown = 1;
        inst->xprev = inst->x;
        inst->yprev = inst->y + 16;
        inst->xmove = 0;
        inst->ymove = 0;
        w->g_shadow_surf = -1;
        w->g_buttons = 0;
        inst->block = 0;
        for (int i = 0; i < LONGO_MAX_DOG_INS; i++) inst->ins[i] = -1;
        for (int i = 0; i < inst->length; i++) {
            LongoInst *part = instance_create(
                w, inst->xx, inst->yy + (16.0f * (i + 1)), LONGO_OBJ_DOGPART, 0, 1.0f, 1.0f);
            if (!part) break;
            inst->ins[i] = part->id;
            if (i == 0) {
                part->follow = inst->id;
                part->first = 1;
                part->draw_legs = 1;
            } else {
                part->follow = inst->ins[i - 1];
                if (i >= inst->length - 1) {
                    part->block = 0;
                    part->draw_legs = 1;
                }
            }
        }
        instance_create(w, inst->x, inst->y, LONGO_OBJ_STUPIDBLOCK, 0, 1.0f, 1.0f);
        inst->depth = 0;
        break;
    }

    case LONGO_OBJ_STUPIDBLOCK: {
        LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
        inst->ins[0] = dog ? dog->ins[0] : -1;
        inst->push = 0;
        inst->block = 0;
        inst->image_xscale = 0.75f;
        inst->image_yscale = 0.75f;
        break;
    }

    case LONGO_OBJ_DOGSPAWNER:
        inst->image_index = 0;
        break;

    case LONGO_OBJ_HOLE:
        inst->block = 1; /* event_inherited() from oBlock */
        inst->full = 0;
        inst->depth = 200;
        break;

    case LONGO_OBJ_DOOR:
        inst->open = 0;
        inst->push = 0;
        inst->block = 1;
        inst->depth = 100;
        break;

    case LONGO_OBJ_FLOWER:
        inst->depth = 200;
        break;

    case LONGO_OBJ_SHADOWS:
        inst->depth = 210;
        break;

    case LONGO_OBJ_GOAL: {
        LongoInst *up = instance_create(w, 0, 0, LONGO_OBJ_GOALUP, -400, 1.0f, 1.0f);
        inst->block = 1;
        inst->push = 0;
        inst->depth = -180;
        (void)up;
        break;
    }

    case LONGO_OBJ_GOALUP:
        inst->remain = 1;
        inst->count = 200;
        inst->count2 = 0;
        inst->depth = -220;
        inst->can_play_sound = 1;
        break;

    case LONGO_OBJ_ONE:
        inst->depth = -200000;
        break;

    case LONGO_OBJ_SMOKE:
        inst->angle = g_random_range(w, -20, 20);
        inst->image_angle = g_random(w, 360);
        inst->dir = inst->image_angle; /* direction */
        inst->speed = g_random(w, 1);
        break;

    case LONGO_OBJ_TITLE:
        play_sound(w, LONGO_SND_PLACEHOLDER, 1);
        w->music_started = 1;
        inst->x = (304.0f / 4.0f) - 20;
        inst->y = (208.0f / 4.0f) - 20;
        inst->xstart = inst->x;
        inst->ystart = inst->y;
        inst->wave1 = 0;
        inst->wave2 = 0;
        {
            LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
            if (dog) {
                float px[5], py[5];
                dog->sprite_index = LONGO_SPR_DOGDOWN;
                dog->dir = 0;
                dog->xx = 168;
                dog->yy = 168;
                dog->x = 168;
                dog->y = 168;
                px[0] = 168; py[0] = dog->yy - 16;
                px[1] = px[0] - 16; py[1] = py[0];
                px[2] = px[1] - 16; py[2] = py[1];
                px[3] = px[2]; py[3] = py[2] + 16;
                px[4] = px[3] + 16; py[4] = py[3];
                for (int i = 0; i < 5; i++) {
                    LongoInst *part = longo_find_id(w, dog->ins[i]);
                    if (!part) continue;
                    part->xx = px[i];
                    part->yy = py[i];
                }
                dog->alarm0 = 10;
            }
        }
        break;

    case LONGO_OBJ_TRANSITION:
        inst->open_transition = 0;
        inst->close_transition = 0;
        inst->retry = 0;
        inst->next_lvl = 0;
        inst->depth = -1000;
        inst->text_y = -16;
        inst->room_num = 1;
        inst->menu = 0;
        inst->volume = 10;
        break;

    case LONGO_OBJ_TUTORIAL: {
        inst->i = 0;
        inst->num = 6;
        int room = w->room_index; /* runtime order: 1 tutorial, 2 level6, 3 level5,
                                     4 level4, 5 level2, 6 level1, 7 level3, 8 credits */
        if (room != 8) { /* rm_credits */
            inst->xscale = 4;
            inst->yscale = 2;
            {
                LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
                if (dog) dog->play = 0;
            }
        } else {
            inst->xscale = 10;
            inst->yscale = 2;
        }
        memset(inst->dbox, 0, sizeof(inst->dbox));
        if (room == 1) { /* rm_tutorial */
            LongoInst *skull = longo_find_first(w, LONGO_OBJ_SKULL);
            inst->dbox[0] = (LongoDbox){ 103, 80, "This is Longo Doggo" };
            inst->dbox[1] = (LongoDbox){ 136, 56,
                "He wants to enter his house, but he's too long so there's no room for him" };
            inst->dbox[2] = (LongoDbox){ 198, 119, "Apples make Longo Doggo longer" };
            inst->dbox[3] = (LongoDbox){ skull ? skull->x - 8.0f : 72, 119,
                "Instead, pears make him shorter" };
            inst->dbox[4] = (LongoDbox){ 144, 56,
                "The house number shows how many length units you must lose" };
            inst->dbox[5] = (LongoDbox){ 103, 80, "If you get stuck press 'R' to retry" };
            inst->dbox[6] = (LongoDbox){ 103, 80,
                "Control Longo Doggo with the arrow keys" };
            inst->num = 6;
        } else if (room == 2) { /* rm_level6 */
            inst->dbox[0] = (LongoDbox){ 247, 51, "This is a door" };
            inst->dbox[1] = (LongoDbox){ 174, 120,
                "It only opens once all the buttons are pressed simultaneously" };
            inst->dbox[2] = (LongoDbox){ 102, 65,
                "Buttons can be pressed either by Longo Doggo or boxes, which you can push on the sides" };
            inst->num = 2;
        } else if (room == 4) { /* rm_level4 */
            inst->dbox[0] = (LongoDbox){ 119, 37,
                "Holes will prevent you from advancing unless you fill them with something" };
            inst->num = 0;
        } else if (room == 8) { /* rm_credits */
            inst->dbox[0] = (LongoDbox){ 151, 37,
                "Thank you for playing! \n \n Game made by Romeu Esteve (@Romeuski) for the 'Tu juego a juicio Jam 2021' \n Using Game Maker Studio 2, freesound.org and Ableton Live 10" };
            inst->dbox[1] = (LongoDbox){ 151, 37,
                "If you enjoyed the experience please leave a comment in the itch.io page, I love feedback!" };
            inst->num = 1;
        }
        break;
    }

    case LONGO_OBJ_MOUSE: {
        static const int palette_preview[10] = { 13, 23, 10, 11, 0, 2, 7, 28, 1, 25 };
        static const int palette_place[10] = { 23, 7, 1, 24, 4, 5, 3, 19, 10, 6 };
        inst->num = 0;
        inst->xx = 0;
        inst->yy = 0;
        for (int i = 0; i < 10; i++) {
            inst->object_palette[i][0] = palette_place[i];
            inst->object_palette[i][1] = palette_preview[i];
        }
        w->g_playing = 0;
        for (int i = 0; i < LONGO_ROOM_GRID; i++)
            for (int j = 0; j < LONGO_ROOM_GRID; j++) inst->level[i][j] = 0;
        break;
    }

    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Alarm events                                                        */
/* ------------------------------------------------------------------ */
static void alarm0_event(LongoWorld *w, LongoInst *inst)
{
    switch (inst->object) {
    case LONGO_OBJ_BUTTERFLY:
        if (inst_distance_to_point(inst, inst->xstart, inst->ystart) < 6) {
            inst->dir = g_random(w, 360);
        } else {
            inst->dir = g_point_direction(inst->x, inst->y, inst->xstart, inst->ystart);
            inst->hspd -= 0.003f;
            inst->vspd -= 0.003f;
        }
        inst->alarm0 = (int)g_random_range(w, 10, 60);
        break;

    case LONGO_OBJ_BUTTON:
        inst->sprite_index = (LongoSprite)inst->sprite;
        inst->alarm0 = 2;
        break;

    case LONGO_OBJ_DOG:
        /* randomize(); */
        if (g_random(w, 1) < 0.2f) {
            play_sound(w, LONGO_SND_BARK, 0);
            LongoInst *bark = instance_create(
                w, inst->x - g_lengthdir_x(12, inst->dir + 90),
                inst->y - g_lengthdir_y(12, inst->dir + 90), LONGO_OBJ_BARK, -500, 1.0f, 1.0f);
            if (bark) bark->image_angle = inst->dir + 180;
        }
        inst->alarm0 = 25;
        break;

    case LONGO_OBJ_DOGPART:
        inst->legs_angle = 0;
        break;

    default:
        break;
    }
}

static void alarm1_event(LongoWorld *w, LongoInst *inst)
{
    switch (inst->object) {
    case LONGO_OBJ_BUTTON:
        play_sound(w, LONGO_SND_WRONG, 0);
        break;
    case LONGO_OBJ_DOG:
        inst->key_cooldown = 1;
        break;
    default:
        break;
    }
    (void)w;
}

/* ------------------------------------------------------------------ */
/* MoveDogX / MoveDogY global scripts                                  */
/* ------------------------------------------------------------------ */
static void move_dog_x(LongoWorld *w, LongoInst *dog)
{
    dog->key_cooldown = 0;
    dog->alarm1 = 2;
    dog->block = 0;
    dog->xprev = dog->x;
    dog->yprev = dog->y;
    dog->x += (16 * dog->xmove);
    dog->yy = dog->y;
    if (dog->xmove > 0) {
        dog->sprite_index = LONGO_SPR_DOGRIGHT;
        dog->dir = 90;
    } else {
        dog->sprite_index = LONGO_SPR_DOGLEFT;
        dog->dir = 270;
    }
    /* TheBullshitDogCode() is empty in the original. */
}

static void move_dog_y(LongoWorld *w, LongoInst *dog)
{
    dog->key_cooldown = 0;
    dog->alarm1 = 2;
    dog->block = 0;
    dog->xprev = dog->x;
    dog->yprev = dog->y;
    dog->y += (16 * dog->ymove);
    dog->xx = dog->x;
    if (dog->ymove > 0) {
        dog->sprite_index = LONGO_SPR_DOGDOWN;
        dog->dir = 0;
    } else {
        dog->sprite_index = LONGO_SPR_DOGUP;
        dog->dir = 180;
    }
}

/* ------------------------------------------------------------------ */
/* Step events                                                         */
/* ------------------------------------------------------------------ */
static void step_dog(LongoWorld *w, LongoInst *dog)
{
    const LongoInput *in = &w->input;

    if (dog->play) {
        dog->xmove = g_sign(((in->vk_right - in->vk_left) + in->key_d) - in->key_a);
        dog->ymove = g_sign((-in->vk_up + in->vk_down + in->key_s) - in->key_w);
        if (in->key_space) {
            play_sound(w, LONGO_SND_BARK, 0);
            LongoInst *bark = instance_create(
                w, dog->xx - g_lengthdir_x(12, dog->dir + 90),
                dog->yy - g_lengthdir_y(12, dog->dir + 90), LONGO_OBJ_BARK, -500, 1.0f, 1.0f);
            if (bark) bark->image_angle = dog->dir + 180;
        }
        LongoInst *trans = longo_find_first(w, LONGO_OBJ_TRANSITION);
        if (in->key_r && !(trans && trans->close_transition)) {
            if (!trans) trans = instance_create(w, 0, 0, LONGO_OBJ_TRANSITION, 0, 1.0f, 1.0f);
            if (trans) {
                trans->retry = 1;
                trans->open_transition = 1;
            }
        }
    }
    if (dog->xmove != 0 && dog->key_cooldown) {
        /* snap boxes to their logical cells before the probe */
        for (int i = 0; i < w->instance_count; i++) {
            LongoInst *box = &w->instances[i];
            if (box->alive && box->object == LONGO_OBJ_BOX) {
                box->x = box->xx;
                box->y = box->yy;
            }
        }
        if (place_meeting(w, dog, dog->x + (10 * dog->xmove), dog->y,
                          LONGO_OBJ_BLOCK)) {
            LongoInst *hit = instance_place(w, dog, dog->x + (10 * dog->xmove),
                                            dog->y, LONGO_OBJ_BLOCK);
            if (hit && hit->block) {
                dog->block = 1;
            } else {
                LongoInst *pushed = instance_place(w, dog,
                                                   dog->x + (8 * dog->xmove),
                                                   dog->y, LONGO_OBJ_BLOCK);
                if (pushed && pushed->push) {
                    pushed->xx += dog->xmove * 16;
                    play_sound(w, LONGO_SND_PUSHED, 0);
                }
                move_dog_x(w, dog);
            }
        } else {
            move_dog_x(w, dog);
        }
    } else if (dog->ymove != 0 && dog->key_cooldown) {
        for (int i = 0; i < w->instance_count; i++) {
            LongoInst *box = &w->instances[i];
            if (box->alive && box->object == LONGO_OBJ_BOX) {
                box->x = box->xx;
                box->y = box->yy;
            }
        }
        if (place_meeting(w, dog, dog->x, dog->y + (10 * dog->ymove),
                          LONGO_OBJ_BLOCK)) {
            LongoInst *hit = instance_place(w, dog, dog->x,
                                            dog->y + (10 * dog->ymove),
                                            LONGO_OBJ_BLOCK);
            if (hit && hit->block) {
                dog->block = 1;
            } else {
                LongoInst *pushed = instance_place(w, dog, dog->x,
                                                   dog->y + (8 * dog->ymove),
                                                   LONGO_OBJ_BLOCK);
                if (pushed && pushed->push) {
                    pushed->yy += dog->ymove * 16;
                    play_sound(w, LONGO_SND_PUSHED, 0);
                }
                move_dog_y(w, dog);
            }
        } else {
            move_dog_y(w, dog);
        }
    }
    if (!longo_instance_exists(w, LONGO_OBJ_TITLE)) {
        dog->xx = g_lerp(dog->xx, dog->x, 0.2f);
        dog->yy = g_lerp(dog->yy, dog->y, 0.2f);
    } else {
        dog->play = 0;
    }
    {
        LongoInst *flower = longo_find_first(w, LONGO_OBJ_FLOWER);
        if (flower) dog->image_index = flower->image_index;
    }
}

static void step_dogpart(LongoWorld *w, LongoInst *part)
{
    LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
    if (dog && part->follow != -4) {
        LongoInst *follower = longo_find_id(w, part->follow);
        if ((dog->xmove != 0 || dog->ymove != 0) && !dog->block &&
            (part->xprev != part->x || part->yprev != part->y) &&
            !dog->key_cooldown) {
            part->xprev = part->x;
            part->yprev = part->y;
            if (part->draw_legs) {
                part->legs_angle = 30;
                part->alarm0 = 15;
            }
        } else if (follower) {
            part->x = follower->xprev;
            part->y = follower->yprev;
        }
        if (!longo_instance_exists(w, LONGO_OBJ_TITLE)) {
            part->xx = g_lerp(part->xx, part->x, 0.2f);
            part->yy = g_lerp(part->yy, part->y, 0.2f);
        }
    } else {
        instance_destroy(part);
    }
}

static void step_stupidblock(LongoWorld *w, LongoInst *sb)
{
    LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
    if (!dog) return;
    float dir = dog->dir + 90;
    sb->x = (dog->x - 6) + g_lengthdir_x(4, dir);
    sb->y = (dog->y - 6) + g_lengthdir_y(4, dir);
}

static void step_box(LongoWorld *w, LongoInst *box)
{
    box->x = g_lerp(box->x, box->xx, 0.25f);
    box->y = g_lerp(box->y, box->yy, 0.25f);
    box->depth = (int)(-100 - box->y / 6);
    int fix = 8;
    LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
    int last_part_id = -1;
    if (dog && dog->length >= 1 && dog->length <= LONGO_MAX_DOG_INS)
        last_part_id = dog->ins[dog->length - 1];

    if (collision_line(w, box->x + fix, box->y + fix, box->x + 16 + fix,
                       box->y + fix, LONGO_OBJ_DOG) &&
        place_meeting(w, box, box->x - 10, box->y, LONGO_OBJ_BLOCK)) {
        LongoInst *hit = instance_place(w, box, box->x - 10, box->y,
                                        LONGO_OBJ_BLOCK);
        if ((hit && hit->id == last_part_id) ||
            place_meeting(w, box, box->x - 10, box->y, LONGO_OBJ_HOLE)) {
            box->block = 0;
        } else {
            box->block = 1;
        }
    } else if (collision_line(w, box->x + fix, box->y + fix,
                              (box->x - 16) + fix, box->y + fix,
                              LONGO_OBJ_DOG) &&
               place_meeting(w, box, box->x + 10, box->y, LONGO_OBJ_BLOCK)) {
        LongoInst *hit = instance_place(w, box, box->x + 10, box->y,
                                        LONGO_OBJ_BLOCK);
        if ((hit && hit->id == last_part_id) ||
            place_meeting(w, box, box->x + 10, box->y, LONGO_OBJ_HOLE)) {
            box->block = 0;
        } else {
            box->block = 1;
        }
    } else if (collision_line(w, box->x + fix, box->y + fix, box->x + fix,
                              box->y + 16 + fix, LONGO_OBJ_DOG) &&
               place_meeting(w, box, box->x, box->y - 10, LONGO_OBJ_BLOCK)) {
        LongoInst *hit = instance_place(w, box, box->x, box->y - 10,
                                        LONGO_OBJ_BLOCK);
        if ((hit && hit->id == last_part_id) ||
            place_meeting(w, box, box->x, box->y - 10, LONGO_OBJ_HOLE)) {
            box->block = 0;
        } else {
            box->block = 1;
        }
    } else if (collision_line(w, box->x + fix, box->y + fix, box->x + fix,
                              (box->y - 16) + fix, LONGO_OBJ_DOG) &&
               place_meeting(w, box, box->x, box->y + 10, LONGO_OBJ_BLOCK)) {
        LongoInst *hit = instance_place(w, box, box->x, box->y + 10,
                                        LONGO_OBJ_BLOCK);
        if ((hit && hit->id == last_part_id) ||
            place_meeting(w, box, box->x, box->y + 10, LONGO_OBJ_HOLE)) {
            box->block = 0;
        } else {
            box->block = 1;
        }
    } else {
        box->block = 0;
    }
}

static void step_button(LongoWorld *w, LongoInst *button)
{
    LongoInst *col = collision_rectangle(w, button->x, button->y,
                                         button->x + 15, button->y + 15,
                                         LONGO_OBJ_BLOCK);
    LongoInst *stupidcol = collision_rectangle(w, button->x, button->y,
                                               button->x + 15, button->y + 15,
                                               LONGO_OBJ_STUPIDBLOCK);
    if (col != NULL || stupidcol != NULL) {
        if (!button->pressed) {
            play_sound(w, LONGO_SND_BUTTON, 0);
            w->g_buttons++;
        }
        button->pressed = 1;
    } else {
        if (button->pressed) {
            play_sound(w, LONGO_SND_WRONG, 0);
            w->g_buttons--;
        }
        button->pressed = 0;
    }
    button->sprite = button->pressed ? 26 : 2;
}

static void step_butterfly(LongoWorld *w, LongoInst *fly)
{
    fly->vspd = g_clamp(fly->vspd, -0.3f, 0.3f);
    fly->hspd = g_clamp(fly->hspd, -0.3f, 0.3f);
    int move;
    if (longo_instance_exists(w, LONGO_OBJ_MOUSE)) {
        move = w->g_playing;
    } else {
        move = 1;
    }
    if (move) {
        fly->hspd += g_lengthdir_x(0.001f, fly->dir);
        fly->vspd += g_lengthdir_y(0.001f, fly->dir);
        fly->x += fly->hspd;
        fly->y += fly->vspd;
    }
    LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
    if (dog && inst_distance(fly, dog) < 10 && w->input.key_space) {
        fly->dir = g_point_direction(dog->x, dog->y, fly->x + 8, fly->y + 8);
        fly->hspd = g_lengthdir_x(0.3f, fly->dir);
        fly->vspd = g_lengthdir_y(0.3f, fly->dir);
    }
}

static void step_housespawner(LongoWorld *w, LongoInst *spawner)
{
    int spawn = 0;
    if (longo_instance_exists(w, LONGO_OBJ_MOUSE)) {
        spawn = w->g_playing;
    } else {
        spawn = 1;
    }
    if (spawn) {
        instance_create(w, spawner->x + 8, spawner->y + 16, LONGO_OBJ_GOAL, 0, 1.0f, 1.0f);
        LongoInst *win = instance_create(w, spawner->x - 8, spawner->y + 16,
                                         LONGO_OBJ_WIN, 0, 1.0f, 1.0f);
        if (win) win->image_xscale = 2;
        instance_destroy(spawner);
    }
}

static void step_dogspawner(LongoWorld *w, LongoInst *spawner)
{
    if (w->g_playing) {
        instance_create(w, spawner->x + 8, spawner->y + 8, LONGO_OBJ_DOG, 0, 1.0f, 1.0f);
        instance_destroy(spawner);
    }
}

static void step_one(LongoWorld *w, LongoInst *one)
{
    one->y = g_lerp(one->y, one->ystart - 16, 0.1f);
    one->image_alpha -= 0.02f;
    if (one->image_alpha <= 0.1f) instance_destroy(one);
}

static void step_smoke_draw_mutations(LongoWorld *w, LongoInst *smoke)
{
    /* oSmoke Draw event side effects (visual part in the renderer). */
    smoke->image_angle += smoke->angle;
    smoke->image_xscale = g_lerp(smoke->image_xscale, 0, 0.04f);
    smoke->image_yscale = g_lerp(smoke->image_yscale, 0, 0.04f);
    smoke->speed = g_lerp(smoke->speed, 0, 0.02f);
    if (smoke->image_xscale < 0.05f) instance_destroy(smoke);
    (void)w;
}

static void step_mouse(LongoWorld *w, LongoInst *mouse)
{
    mouse->x = w->input.mouse_x;
    mouse->y = w->input.mouse_y;
    mouse->xx = floorf(mouse->x / 16) * 16;
    mouse->yy = floorf(mouse->y / 16) * 16;
    if (w->input.mb_left) {
        int place_object = mouse->object_palette[mouse->num][0];
        instance_create(w, mouse->xx, mouse->yy, place_object, 0, 1.0f, 1.0f);
    }
    if (w->input.mb_right) {
        LongoInst *hit = NULL;
        float ax1, ay1, ax2, ay2;
        inst_bbox(mouse, mouse->x, mouse->y, &ax1, &ay1, &ax2, &ay2);
        for (int i = 0; i < w->instance_count; i++) {
            LongoInst *other = &w->instances[i];
            float bx1, by1, bx2, by2;
            if (!other->alive) continue;
            if (other->sprite_index == LONGO_SPR_NONE) continue;
            inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
            if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2)) {
                hit = other;
                break;
            }
        }
        if (hit && hit->object != LONGO_OBJ_DOGPART) {
            instance_destroy(hit);
        }
    }
}

static void step_event(LongoWorld *w, LongoInst *inst)
{
    switch (inst->object) {
    case LONGO_OBJ_DOG: step_dog(w, inst); break;
    case LONGO_OBJ_DOGPART: step_dogpart(w, inst); break;
    case LONGO_OBJ_STUPIDBLOCK: step_stupidblock(w, inst); break;
    case LONGO_OBJ_BOX: step_box(w, inst); break;
    case LONGO_OBJ_BUTTON: step_button(w, inst); break;
    case LONGO_OBJ_BUTTERFLY: step_butterfly(w, inst); break;
    case LONGO_OBJ_HOUSESPAWNER: step_housespawner(w, inst); break;
    case LONGO_OBJ_DOGSPAWNER: step_dogspawner(w, inst); break;
    case LONGO_OBJ_ONE: step_one(w, inst); break;
    case LONGO_OBJ_MOUSE: step_mouse(w, inst); break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* Collision events                                                    */
/* ------------------------------------------------------------------ */
static void collision_dog_apple(LongoWorld *w, LongoInst *dog, LongoInst *apple)
{
    dog->length++;
    LongoInst *prev = longo_find_id(w, dog->ins[dog->length - 2]);
    if (prev) {
        prev->block = 1;
        prev->draw_legs = 0;
        LongoInst *part = instance_create(w, prev->xprev, prev->yprev,
                                          LONGO_OBJ_DOGPART, 0, 1.0f, 1.0f);
        if (part) {
            dog->ins[dog->length - 1] = part->id;
            part->follow = prev->id;
            part->block = 0;
            part->draw_legs = 1;
            for (int i = 0; i < 7; i++) {
                LongoInst *smoke = instance_create(
                    w, part->x + g_random_range(w, -3, 3),
                    part->y + g_random_range(w, -3, 3), LONGO_OBJ_SMOKE, -1000, 1.0f, 1.0f);
                (void)smoke;
            }
        }
    }
    instance_create(w, dog->x, dog->y - 8, LONGO_OBJ_ONE, -1000, 1.0f, 1.0f);
    play_sound(w, LONGO_SND_POOF, 0);
    instance_destroy(apple);
}

static void collision_dog_skull(LongoWorld *w, LongoInst *dog, LongoInst *skull)
{
    if (dog->length > 2) {
        dog->length--;
        LongoInst *tail = longo_find_id(w, dog->ins[dog->length]);
        if (tail) {
            for (int i = 0; i < 7; i++) {
                instance_create(w, tail->x + g_random_range(w, -3, 3),
                                tail->y + g_random_range(w, -3, 3),
                                LONGO_OBJ_SMOKE, -1000, 1.0f, 1.0f);
            }
            instance_destroy(tail);
        }
        {
            LongoInst *one = instance_create(w, dog->x, dog->y - 8,
                                             LONGO_OBJ_ONE, -1000, 1.0f, 1.0f);
            if (one) one->image_index = 1;
        }
        dog->ins[dog->length] = -1;
        LongoInst *prev = longo_find_id(w, dog->ins[dog->length - 1]);
        if (prev) {
            prev->block = 0;
            prev->draw_legs = 1;
        }
    } else {
        instance_destroy(dog);
    }
    play_sound(w, LONGO_SND_POOF, 0);
    instance_destroy(skull);
}

static void collision_hole_box(LongoWorld *w, LongoInst *hole, LongoInst *box)
{
    if (hole->image_index != 1) {
        hole->full = 1;
        for (int i = 0; i < 7; i++) {
            instance_create(w, hole->x + 4 + g_random(w, 8),
                            hole->y + 4 + g_random(w, 8), LONGO_OBJ_SMOKE, -200,
                            1.0f, 1.0f);
        }
        hole->block = 0;
        hole->sprite_index = LONGO_SPR_NONE;
        play_sound(w, LONGO_SND_POOF, 0);
        instance_destroy(box);
    }
}

static void collision_win_dog(LongoWorld *w, LongoInst *win, LongoInst *dog)
{
    (void)dog;
    LongoInst *goalup = longo_find_first(w, LONGO_OBJ_GOALUP);
    if (goalup && goalup->remain <= 0) {
        if (!longo_instance_exists(w, LONGO_OBJ_MOUSE)) {
            LongoInst *trans = longo_find_first(w, LONGO_OBJ_TRANSITION);
            if (trans) {
                trans->room_num++;
                trans->next_lvl = 1;
                trans->open_transition = 1;
            }
            instance_destroy(win);
        } else {
            /* keyboard_key_press(vk_enter): editor replay hook, no-op here */
        }
    }
}

static void collision_events(LongoWorld *w)
{
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive) continue;

        if (inst->object == LONGO_OBJ_DOG) {
            /* collision events run per class in object-index order:
             * oApple (7) before oSkull (1)?  GameMaker orders by event
             * registration; recovered order keeps apples then skulls. */
            for (int j = 0; j < w->instance_count; j++) {
                LongoInst *other = &w->instances[j];
                float ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
                if (!other->alive || other->object != LONGO_OBJ_APPLE) continue;
                inst_bbox(inst, inst->x, inst->y, &ax1, &ay1, &ax2, &ay2);
                inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
                if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2))
                    collision_dog_apple(w, inst, other);
            }
            if (!inst->alive) continue;
            for (int j = 0; j < w->instance_count; j++) {
                LongoInst *other = &w->instances[j];
                float ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
                if (!other->alive || other->object != LONGO_OBJ_SKULL) continue;
                inst_bbox(inst, inst->x, inst->y, &ax1, &ay1, &ax2, &ay2);
                inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
                if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2))
                    collision_dog_skull(w, inst, other);
            }
        } else if (inst->object == LONGO_OBJ_HOLE) {
            for (int j = 0; j < w->instance_count; j++) {
                LongoInst *other = &w->instances[j];
                float ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
                if (!other->alive || other->object != LONGO_OBJ_BOX) continue;
                inst_bbox(inst, inst->x, inst->y, &ax1, &ay1, &ax2, &ay2);
                inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
                if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2))
                    collision_hole_box(w, inst, other);
            }
        } else if (inst->object == LONGO_OBJ_WIN) {
            for (int j = 0; j < w->instance_count; j++) {
                LongoInst *other = &w->instances[j];
                float ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
                if (!other->alive || other->object != LONGO_OBJ_DOG) continue;
                inst_bbox(inst, inst->x, inst->y, &ax1, &ay1, &ax2, &ay2);
                inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
                if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2))
                    collision_win_dog(w, inst, other);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Draw-event side effects (GameMaker Draw events that mutate state)   */
/* ------------------------------------------------------------------ */
static void draw_door_mutations(LongoWorld *w, LongoInst *door)
{
    if (!longo_instance_exists(w, LONGO_OBJ_MOUSE)) {
        if (door->open) {
            door->image_xscale = g_lerp(door->image_xscale, 1.2f, 0.1f);
            door->image_yscale = g_lerp(door->image_yscale, 0.8f, 0.1f);
            float sprite_width = 16.0f * door->image_xscale;
            float sprite_height = 16.0f * door->image_yscale;
            door->x = door->xstart - (sprite_width * (door->image_xscale - 1)) / 2;
            door->y = door->ystart - (sprite_height * (door->image_yscale - 1));
            if (door->image_xscale > 1.15f) {
                for (int i = 0; i < 7; i++) {
                    instance_create(w, door->x + g_random(w, 16),
                                    door->y + g_random(w, 16), LONGO_OBJ_SMOKE,
                                    -1000, 1.0f, 1.0f);
                }
                play_sound(w, LONGO_SND_POOF, 0);
                instance_destroy(door);
                return;
            }
        }
    } else if (w->g_playing) {
        if (w->g_buttons == longo_instance_number(w, LONGO_OBJ_BUTTON)) {
            door->image_xscale = g_lerp(door->image_xscale, 1.2f, 0.1f);
            door->image_yscale = g_lerp(door->image_yscale, 0.8f, 0.1f);
            float sprite_width = 16.0f * door->image_xscale;
            float sprite_height = 16.0f * door->image_yscale;
            door->x = door->xstart - (sprite_width * (door->image_xscale - 1)) / 2;
            door->y = door->ystart - (sprite_height * (door->image_yscale - 1));
            if (door->image_xscale > 1.15f) {
                for (int i = 0; i < 7; i++) {
                    instance_create(w, door->x + g_random(w, 16),
                                    door->y + g_random(w, 16), LONGO_OBJ_SMOKE,
                                    -1000, 1.0f, 1.0f);
                }
                play_sound(w, LONGO_SND_POOF, 0);
                instance_destroy(door);
                return;
            }
        }
    }
    if (w->g_buttons == longo_instance_number(w, LONGO_OBJ_BUTTON)) {
        door->open = 1;
    }
}

static void draw_goalup_mutations(LongoWorld *w, LongoInst *goalup)
{
    LongoInst *goal = longo_find_first(w, LONGO_OBJ_GOAL);
    LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
    if (!goal) return;
    int goal_length = 2;
    if (dog) goalup->remain = dog->length - goal_length;
    if (goalup->remain > 0) {
        goalup->can_play_sound = 1;
        goal->image_index = 0;
        goalup->count = 200;
        if (goalup->count2 > 0) {
            if (goalup->count2 > 160) {
                instance_create(w, goal->x + g_random_range(w, -4, 4),
                                (goal->y - 8) + g_random_range(w, -4, 4),
                                LONGO_OBJ_SMOKE, -1000, 1.0f, 1.0f);
            }
            goalup->count2 -= 4;
            goal->image_xscale = 1 + longo_wave(-goalup->count2 / 1000.0f,
                                                goalup->count2 / 1000.0f, 0.35f,
                                                0, w->current_time_ms);
            goal->image_yscale = 1 - longo_wave(-goalup->count2 / 1000.0f,
                                                goalup->count2 / 1000.0f, 0.35f,
                                                0, w->current_time_ms);
        }
    } else {
        if (goalup->can_play_sound) {
            play_sound(w, LONGO_SND_WIN, 0);
            goalup->can_play_sound = 0;
        }
        if (goalup->count > 0) {
            if (goalup->count > 160) {
                instance_create(w, goal->x + g_random_range(w, -4, 4),
                                (goal->y - 8) + g_random_range(w, -4, 4),
                                LONGO_OBJ_SMOKE, -1000, 1.0f, 1.0f);
            }
            goalup->count -= 4;
            goal->image_xscale = 1 + longo_wave(-goalup->count / 1000.0f,
                                                goalup->count / 1000.0f, 0.35f,
                                                0, w->current_time_ms);
            goal->image_yscale = 1 - longo_wave(-goalup->count / 1000.0f,
                                                goalup->count / 1000.0f, 0.35f,
                                                0, w->current_time_ms);
        }
        goal->image_index = 1;
        goalup->count2 = 200;
    }
}

static void draw_title_mutations(LongoWorld *w, LongoInst *title)
{
    title->wave1 = longo_wave(0, 8, 2, 0, w->current_time_ms);
    title->wave2 = longo_wave(0, 8, 2, 0.1f, w->current_time_ms);
    if (w->input.vk_anykey) {
        LongoInst *trans = longo_find_first(w, LONGO_OBJ_TRANSITION);
        if (!trans) trans = instance_create(w, 0, 0, LONGO_OBJ_TRANSITION, 0, 1.0f, 1.0f);
        if (trans) {
            trans->next_lvl = 1;
            trans->open_transition = 1;
        }
    }
}

static void draw_tutorial_mutations(LongoWorld *w, LongoInst *tut)
{
    float wave = longo_wave(0, 2, 2, 0, w->current_time_ms);
    (void)wave;
    LongoInst *trans = longo_find_first(w, LONGO_OBJ_TRANSITION);
    if ((w->input.key_space || w->input.key_enter || w->input.key_e) &&
        !(trans && trans->close_transition)) {
        tut->image_xscale = 0.5f;
        tut->image_yscale = 0.5f;
        if (tut->i < tut->num) {
            tut->i++;
        } else {
            LongoInst *dog = longo_find_first(w, LONGO_OBJ_DOG);
            if (dog) dog->play = 1;
            tut->xscale = 0.6f;
            tut->yscale = 0.6f;
        }
    }
    tut->image_xscale = g_lerp(tut->image_xscale, tut->xscale, 0.15f);
    tut->image_yscale = g_lerp(tut->image_yscale, tut->yscale, 0.15f);
    if (tut->image_yscale < 0.65f && tut->i >= tut->num) {
        instance_destroy(tut);
    }
}

static void draw_transition_mutations(LongoWorld *w, LongoInst *trans)
{
    if (trans->open_transition) {
        if (trans->x >= -20) {
            trans->x = g_lerp(trans->x, -31, 0.04f);
        } else {
            if (!trans->menu) trans->x = 304;
            if (!trans->menu) trans->close_transition = 1;
            if (trans->retry) {
                longo_room_restart(w);
                trans->retry = 0;
            } else if (trans->next_lvl) {
                longo_room_goto_next(w);
                trans->next_lvl = 0;
            }
            if (!trans->menu) trans->open_transition = 0;
        }
        if (trans->room_num <= 7 && trans->menu == 0) {
            trans->text_y = g_lerp(trans->text_y, 208.0f / 2 + 4, 0.05f);
        } else if (trans->menu) {
            trans->text_y = g_lerp(trans->text_y, 208.0f / 2 + 4, 0.05f);
        }
    } else if (trans->close_transition) {
        /* GML: for (i = 0; i < 208/64; i++) with real division -> 4 passes,
         * so the lerp applies four times per frame. */
        for (int i = 0; i < 4; i++) {
            if (trans->x >= -63) {
                trans->x = g_lerp(trans->x, -64, 0.02f);
            } else {
                trans->close_transition = 0;
            }
        }
        if (trans->room_num <= 7) {
            trans->text_y = g_lerp(trans->text_y, 208.0f + 16, 0.16f);
        }
    } else {
        trans->x = 304;
        trans->text_y = -16;
    }
}

static void draw_mouse_mutations(LongoWorld *w, LongoInst *mouse)
{
    if (w->input.mouse_wheel > 0) {
        mouse->num = mouse->num < 9 ? mouse->num + 1 : 0;
    } else if (w->input.mouse_wheel < 0) {
        mouse->num = mouse->num > 0 ? mouse->num - 1 : 9;
    }
}

/* oMouse KeyPress <Enter> (event 13) */
static void mouse_enter_event(LongoWorld *w, LongoInst *mouse)
{
    if (!w->g_playing) {
        for (int i = 0; i < (int)(w->room->width / 16) + 1 &&
                        i < LONGO_ROOM_GRID;
             i++) {
            for (int j = 0; j < (int)(w->room->height / 16) + 1 &&
                            j < LONGO_ROOM_GRID;
                 j++) {
                LongoInst *hit = NULL;
                float ax1, ay1, ax2, ay2;
                inst_bbox(mouse, (i * 16) - 8, (j * 16) - 8, &ax1, &ay1, &ax2,
                          &ay2);
                for (int k = 0; k < w->instance_count; k++) {
                    LongoInst *other = &w->instances[k];
                    float bx1, by1, bx2, by2;
                    if (!other->alive || other == mouse) continue;
                    if (other->sprite_index == LONGO_SPR_NONE) continue;
                    inst_bbox(other, other->x, other->y, &bx1, &by1, &bx2, &by2);
                    if (rects_overlap(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2)) {
                        hit = other;
                        break;
                    }
                }
                mouse->level[i][j] = hit ? hit->object : -4;
            }
        }
    } else {
        for (int i = 0; i < w->instance_count; i++) {
            LongoInst *other = &w->instances[i];
            if (other->alive && other->id != mouse->id) instance_destroy(other);
        }
        instance_create(w, (float)w->room->width + 8, 0, LONGO_OBJ_FLOWER, 0,
                        1.0f, 1.0f);
        w->g_buttons = 0;
        for (int i = 0; i < (int)(w->room->width / 16) + 1 &&
                        i < LONGO_ROOM_GRID;
             i++) {
            for (int j = 0; j < (int)(w->room->height / 16) + 1 &&
                            j < LONGO_ROOM_GRID;
                 j++) {
                if (mouse->level[i][j] != -4) {
                    instance_create(w, (float)(i * 16) - 16,
                                    (float)(j * 16) - 16, mouse->level[i][j], 0,
                                    1.0f, 1.0f);
                }
            }
        }
    }
    w->g_playing = !w->g_playing;
}

/* ------------------------------------------------------------------ */
/* Room management                                                     */
/* ------------------------------------------------------------------ */
static void load_room_instances(LongoWorld *w, int room_index)
{
    const LongoRoom *room = longo_rooms[room_index];
    w->room_index = room_index;
    w->room = room;

    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        int persistent = inst->object == LONGO_OBJ_TRANSITION ||
                         inst->object == LONGO_BLOOM ||
                         inst->object == LONGO_OBJ_POSTEFFECTS;
        if (!persistent) inst->alive = 0;
    }

    for (int i = 0; i < room->instance_count; i++) {
        const LongoRoomInstance *placed = &room->instances[i];
        LongoInst *inst = instance_create(w, placed->x, placed->y, placed->object,
                                          placed->depth, placed->xscale,
                                          placed->yscale);
        if (!inst) break;
    }
}

void longo_room_goto(LongoWorld *w, int room_index)
{
    if (room_index < 0 || room_index >= LONGO_ROOM_COUNT) return;
    load_room_instances(w, room_index);
}

void longo_room_goto_next(LongoWorld *w)
{
    longo_room_goto(w, w->room_index + 1); /* out of range keeps current room */
}

void longo_room_restart(LongoWorld *w)
{
    longo_room_goto(w, w->room_index);
}

void longo_init(LongoWorld *w, unsigned int seed)
{
    memset(w, 0, sizeof(*w));
    w->rng = seed ? seed : 0x1234u;
    w->current_tick = 1; /* tick 0 means "created this tick" */
    w->next_id = 0;
    load_room_instances(w, 0);
}

int longo_poll_sound(LongoWorld *w, LongoSoundEvent *out)
{
    int n = w->sound_count;
    if (n > 0 && out) memcpy(out, w->sounds, sizeof(LongoSoundEvent) * (size_t)n);
    w->sound_count = 0;
    return n;
}

/* ------------------------------------------------------------------ */
/* One GameMaker frame                                                 */
/* ------------------------------------------------------------------ */
void longo_tick(LongoWorld *w, const LongoInput *input, double delta_ms)
{
    if (input) w->input = *input;
    else memset(&w->input, 0, sizeof(w->input));
    w->current_time_ms += delta_ms;
    w->sound_count = 0;
    w->current_tick++;

    /* 1. Alarm events */
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive) continue;
        if (inst->alarm0 > 0) {
            inst->alarm0--;
            if (inst->alarm0 == 0) {
                inst->alarm0 = -1;
                alarm0_event(w, inst);
                if (!inst->alive) continue;
            }
        }
        if (inst->alarm1 > 0) {
            inst->alarm1--;
            if (inst->alarm1 == 0) {
                inst->alarm1 = -1;
                alarm1_event(w, inst);
            }
        }
    }

    /* 2. Step events (creation order; instances born this tick skip) */
    int count = w->instance_count;
    for (int i = 0; i < count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive || inst->born_tick == w->current_tick) continue;
        step_event(w, inst);
    }

    /* oMouse KeyPress <Enter> runs between alarms and steps in GameMaker;
     * applying it here keeps its grid scan consistent with the frame. */
    if (w->input.key_enter) {
        for (int i = 0; i < w->instance_count; i++) {
            LongoInst *inst = &w->instances[i];
            if (inst->alive && inst->object == LONGO_OBJ_MOUSE &&
                inst->born_tick != w->current_tick) {
                mouse_enter_event(w, inst);
            }
        }
    }

    /* 3. Collision events */
    collision_events(w);

    /* 4. Built-in motion (speed/direction) */
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive) continue;
        if (inst->object == LONGO_OBJ_SMOKE) {
            inst->x += g_lengthdir_x(inst->speed, inst->dir);
            inst->y += g_lengthdir_y(inst->speed, inst->dir);
        }
    }

    /* 5. Draw-event side effects, in draw order (depth desc, id asc).
     * A full sort is unnecessary: each mutation only reads its own state
     * plus dogs/goals/buttons, so id order matches GameMaker closely. */
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive || inst->born_tick == w->current_tick) continue;
        switch (inst->object) {
        case LONGO_OBJ_DOOR: draw_door_mutations(w, inst); break;
        case LONGO_OBJ_GOALUP: draw_goalup_mutations(w, inst); break;
        case LONGO_OBJ_SMOKE: step_smoke_draw_mutations(w, inst); break;
        default: break;
        }
    }
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive || inst->born_tick == w->current_tick) continue;
        switch (inst->object) {
        case LONGO_OBJ_TITLE: draw_title_mutations(w, inst); break;
        case LONGO_OBJ_TUTORIAL: draw_tutorial_mutations(w, inst); break;
        case LONGO_OBJ_TRANSITION: draw_transition_mutations(w, inst); break;
        case LONGO_OBJ_MOUSE: draw_mouse_mutations(w, inst); break;
        default: break;
        }
    }

    /* 6. Image animation advance + Animation End */
    for (int i = 0; i < w->instance_count; i++) {
        LongoInst *inst = &w->instances[i];
        if (!inst->alive || inst->sprite_index == LONGO_SPR_NONE) continue;
        const LongoSpriteInfo *spr = &LONGO_SPRITES[inst->sprite_index];
        if (spr->frames <= 0) continue;
        inst->image_index += inst->image_speed *
                             (float)spr->fps / 60.0f;
        if (inst->image_index >= (float)spr->frames) {
            inst->image_index = fmodf(inst->image_index, (float)spr->frames);
            if (inst->object == LONGO_OBJ_BARK) {
                instance_destroy(inst); /* Animation End event */
            }
        }
    }
}
