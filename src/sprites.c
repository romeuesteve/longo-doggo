#include "sprites.h"

#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

float longo_wave(float a, float b, float period, float phase, double time_ms)
{
    float a4 = (b - a) * 0.5f;
    float t = (float)(time_ms * 0.001);
    return a + a4 + sinf(((t + period * phase) / period) * (2.0f * (float)M_PI)) * a4;
}

static const LongoSpriteInfo SPRITES[] = {
    [LONGO_SPR_BLOCK] = { 16, 16, 0, 0, 2, 0 },
    [LONGO_SPR_HOLE] = { 16, 16, 0, 0, 9, 0 },
    [LONGO_SPR_BUTTON] = { 16, 16, 0, 0, 9, 8 },
    [LONGO_SPR_TRANSITION] = { 64, 64, 0, 0, 2, 30 },
    [LONGO_SPR_DOGPAW] = { 16, 16, 8, 8, 1, 8 },
    [LONGO_SPR_OLDDOG] = { 64, 64, 0, 0, 5, 8 },
    [LONGO_SPR_HOUSE] = { 64, 64, 32, 64, 2, 0 },
    [LONGO_SPR_FLY] = { 16, 16, 0, 0, 8, 16 },
    [LONGO_SPR_DOGUP] = { 16, 16, 8, 8, 4, 8 },
    [LONGO_SPR_BUTTON43] = { 32, 32, 0, 0, 9, 8 },
    [LONGO_SPR_PEAR] = { 16, 16, 0, 0, 8, 16 },
    [LONGO_SPR_BOX] = { 16, 20, 0, 4, 1, 30 },
    [LONGO_SPR_ICON] = { 16, 16, 0, 0, 1, 0 },
    [LONGO_SPR_BARK] = { 16, 16, 8, 8, 7, 18 },
    [LONGO_SPR_TILE] = { 32, 32, 0, 0, 1, 30 },
    [LONGO_SPR_FLOWER] = { 8, 8, 0, 0, 4, 8 },
    [LONGO_SPR_SMOKE] = { 16, 16, 8, 8, 1, 30 },
    [LONGO_SPR_DOGTAIL] = { 16, 16, 8, 8, 4, 8 },
    [LONGO_SPR_TITLE] = { 191, 149, 0, 0, 3, 30 },
    [LONGO_SPR_DOGLEFT] = { 16, 16, 8, 8, 4, 8 },
    [LONGO_SPR_DOGRIGHT] = { 16, 16, 8, 8, 4, 8 },
    [LONGO_SPR_DIALOGUEBOX] = { 24, 24, 12, 12, 1, 30 },
    [LONGO_SPR_APPLE] = { 16, 16, 0, 0, 8, 16 },
    [LONGO_SPR_SKULL] = { 18, 18, 0, 0, 4, 8 },
    [LONGO_SPR_ICON2] = { 16, 16, 0, 0, 1, 30 },
    [LONGO_SPR_BUTTONPRESSED] = { 16, 16, 0, 0, 1, 8 },
    [LONGO_SPR_ONE] = { 8, 8, 0, 0, 2, 0 },
    [LONGO_SPR_DOOR] = { 16, 16, 0, 0, 1, 0 },
    [LONGO_SPR_DOGDOWN] = { 16, 16, 8, 8, 4, 8 },
};

static const char *NAMES[] = {
    [LONGO_SPR_BLOCK] = "sprBlock",
    [LONGO_SPR_HOLE] = "sprHole",
    [LONGO_SPR_BUTTON] = "sprButton",
    [LONGO_SPR_TRANSITION] = "sprTransition",
    [LONGO_SPR_DOGPAW] = "sprDogPaw",
    [LONGO_SPR_OLDDOG] = "sprOldDog",
    [LONGO_SPR_HOUSE] = "sprHouse",
    [LONGO_SPR_FLY] = "sprFly",
    [LONGO_SPR_DOGUP] = "sprDogUp",
    [LONGO_SPR_BUTTON43] = "sprButton43",
    [LONGO_SPR_PEAR] = "sprPear",
    [LONGO_SPR_BOX] = "sprBox",
    [13] = "sprIcon",
    [LONGO_SPR_BARK] = "sprBark",
    [LONGO_SPR_TILE] = "sprTile",
    [LONGO_SPR_FLOWER] = "sprFlower",
    [LONGO_SPR_SMOKE] = "sprSmoke",
    [LONGO_SPR_DOGTAIL] = "sprDogTail",
    [LONGO_SPR_TITLE] = "sprTitle",
    [LONGO_SPR_DOGLEFT] = "sprDogLeft",
    [LONGO_SPR_DOGRIGHT] = "sprDogRight",
    [LONGO_SPR_DIALOGUEBOX] = "sprDialogueBox",
    [LONGO_SPR_APPLE] = "sprApple",
    [LONGO_SPR_SKULL] = "sprSkull",
    [LONGO_SPR_ICON2] = "sprIcon2",
    [LONGO_SPR_BUTTONPRESSED] = "sprButtonPressed",
    [LONGO_SPR_ONE] = "sprOne",
    [LONGO_SPR_DOOR] = "sprDoor",
    [LONGO_SPR_DOGDOWN] = "sprDogDown",
};

#define SPRITE_TABLE_COUNT (int)(sizeof(SPRITES) / sizeof(SPRITES[0]))

const LongoSpriteInfo *longo_sprite_info(int sprite)
{
    if (sprite < 0 || sprite >= SPRITE_TABLE_COUNT) return NULL;
    return &SPRITES[sprite];
}

const char *longo_sprite_name(int sprite)
{
    if (sprite < 0 || sprite >= SPRITE_TABLE_COUNT) return NULL;
    return NAMES[sprite];
}

int longo_sprite_origin_x(int sprite)
{
    const LongoSpriteInfo *info = longo_sprite_info(sprite);
    return info ? info->origin_x : 0;
}

int longo_sprite_origin_y(int sprite)
{
    const LongoSpriteInfo *info = longo_sprite_info(sprite);
    return info ? info->origin_y : 0;
}

int longo_sprite_frames(int sprite)
{
    const LongoSpriteInfo *info = longo_sprite_info(sprite);
    return info ? info->frames : 0;
}
