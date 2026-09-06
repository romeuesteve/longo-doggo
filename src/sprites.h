/*
 * Sprite asset metadata (frame counts, origins, playback speeds) shared
 * by the simulation (placement -> cell mapping) and the renderer (frame
 * advance, origins).  The numeric order matches the exported asset
 * table under assets/exported-assets.
 */
#ifndef LONGO_SPRITES_H
#define LONGO_SPRITES_H

typedef enum LongoSprite {
    LONGO_SPR_BLOCK = 0,
    LONGO_SPR_HOLE = 1,
    LONGO_SPR_BUTTON = 2,
    LONGO_SPR_TRANSITION = 3,
    LONGO_SPR_DOGPAW = 4,
    LONGO_SPR_OLDDOG = 5,
    LONGO_SPR_HOUSE = 6,
    LONGO_SPR_FLY = 7,
    LONGO_SPR_DOGUP = 8,
    LONGO_SPR_BUTTON43 = 9,
    LONGO_SPR_PEAR = 10,
    LONGO_SPR_BOX = 11,
    LONGO_SPR_ICON = 13,
    LONGO_SPR_BARK = 14,
    LONGO_SPR_TILE = 15,
    LONGO_SPR_FLOWER = 16,
    LONGO_SPR_SMOKE = 17,
    LONGO_SPR_DOGTAIL = 18,
    LONGO_SPR_TITLE = 19,
    LONGO_SPR_DOGLEFT = 20,
    LONGO_SPR_DOGRIGHT = 21,
    LONGO_SPR_DIALOGUEBOX = 22,
    LONGO_SPR_APPLE = 23,
    LONGO_SPR_SKULL = 24,
    LONGO_SPR_ICON2 = 25,
    LONGO_SPR_BUTTONPRESSED = 26,
    LONGO_SPR_ONE = 27,
    LONGO_SPR_DOOR = 28,
    LONGO_SPR_DOGDOWN = 29,
    LONGO_SPR_NONE = -1
} LongoSprite;

typedef struct LongoSpriteInfo {
    int width, height;
    int origin_x, origin_y;
    int frames;
    int fps; /* playback speed; the animation clock advances by
                fps / 60 frames per tick */
} LongoSpriteInfo;

const LongoSpriteInfo *longo_sprite_info(int sprite);
const char *longo_sprite_name(int sprite);

int longo_sprite_origin_x(int sprite);
int longo_sprite_origin_y(int sprite);
int longo_sprite_frames(int sprite);

/* Sine wobble shared by the title, goal and dialogue text. */
float longo_wave(float a, float b, float period, float phase, double time_ms);

#endif /* LONGO_SPRITES_H */
