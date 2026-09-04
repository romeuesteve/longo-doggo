/*
 * Longo Doggo - GameMaker-faithful simulation core.
 *
 * This module re-implements the recovered GML object events from
 * recovered/exported-assets/code as a small instance-based runtime.  It has
 * no rendering dependency: the renderer walks longo_world.instances and
 * reproduces the original Draw events.
 *
 * One call to longo_tick() equals one GameMaker frame at the original room
 * speed of 60 FPS, in GameMaker's event order:
 *   alarms -> step events -> collision events -> built-in motion -> draw.
 */
#ifndef LONGO_DOGGO_GAME_H
#define LONGO_DOGGO_GAME_H

#include "level_data.h"

#define LONGO_MAX_INSTANCES 1024
#define LONGO_MAX_DOG_INS 64
#define LONGO_MAX_SOUNDS 64
#define LONGO_ROOM_GRID 24 /* oMouse editor level array bound */

/* Sprite asset indices recovered from data.win (the numeric order matches
 * GameMaker's sprite table; oButton/oMouse reference sprites by number). */
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

/* Sound asset indices recovered from data.win. */
typedef enum LongoSound {
    LONGO_SND_PLACEHOLDER = 0,
    LONGO_SND_POOF = 1,
    LONGO_SND_WRONG = 2,
    LONGO_SND_PUSHED = 3,
    LONGO_SND_WIN = 4,
    LONGO_SND_BUTTON = 5,
    LONGO_SND_BARK = 6
} LongoSound;

typedef struct LongoInput {
    /* keyboard_check_pressed() edges for one tick */
    unsigned char vk_right, vk_left, vk_up, vk_down;
    unsigned char key_d, key_a, key_s, key_w;
    unsigned char key_r, key_space, key_enter, key_e;
    unsigned char vk_anykey;
    /* editor (oMouse) input, in room coordinates */
    float mouse_x, mouse_y;
    unsigned char mb_left, mb_right;
    int mouse_wheel; /* +1 wheel up, -1 wheel down */
} LongoInput;

typedef struct LongoDbox {
    float x, y;
    const char *text;
} LongoDbox;

typedef struct LongoInst {
    int id;
    int object; /* LongoObj */
    int alive;
    int born_tick; /* instances created this tick skip Step (GameMaker) */

    float x, y;
    float xstart, ystart;
    float xprev, yprev;
    int depth;
    LongoSprite sprite_index;
    float image_index;
    float image_speed;
    float image_xscale, image_yscale;
    float image_angle;
    float image_alpha;
    int visible;

    int alarm0, alarm1; /* -1 = inactive */

    /* Per-object state (fields shared across kinds, GML names kept) */
    float dir;       /* oDog/oButterfly direction */
    int play;        /* oDog */
    int length;      /* oDog */
    int lengthstart; /* oDog */
    float xx, yy;    /* smoothed draw position */
    int key_cooldown;
    float xmove, ymove;
    int block, push; /* oBlock family collision flags */
    int ins[LONGO_MAX_DOG_INS]; /* oDog -> oDogPart ids */
    int follow;      /* oDogPart -> follower id */
    int first;       /* oDogPart */
    int draw_legs;   /* oDogPart */
    float legs_angle;
    int pressed;     /* oButton */
    int wait;        /* oButton */
    int sprite;      /* oButton pending sprite asset number */
    int open;        /* oDoor */
    int full;        /* oHole */
    int remain, count, count2, can_play_sound; /* oGoalUp */
    float hspd, vspd; /* oButterfly/oSmoke */
    float angle;      /* oSmoke spin */
    float speed;      /* oSmoke */
    float wave1, wave2; /* oTitle */
    int i, num;       /* oTutorial/oMouse */
    float xscale, yscale; /* oTutorial target box scale */
    LongoDbox dbox[8];
    int open_transition, close_transition, retry, next_lvl; /* oTransition */
    float text_y;
    int room_num, menu, volume;
    int level[LONGO_ROOM_GRID][LONGO_ROOM_GRID]; /* oMouse editor grid */
    int object_palette[10][2];                   /* oMouse palette */
} LongoInst;

typedef struct LongoSoundEvent {
    int sound;
    int loop;
} LongoSoundEvent;

typedef struct LongoWorld {
    LongoInst instances[LONGO_MAX_INSTANCES];
    int instance_count;
    int next_id;

    int room_index; /* index into longo_rooms (runtime order) */
    const LongoRoom *room;
    int current_tick;
    double current_time_ms;

    /* globals */
    int g_playing;      /* global.playing (editor play mode) */
    int g_buttons;      /* global.buttons */
    int g_shadow_surf;  /* global.shadow_surf exists (renderer owns pixels) */
    int g_selected_module;

    LongoInput input;

    LongoSoundEvent sounds[LONGO_MAX_SOUNDS];
    int sound_count;
    int music_started;

    unsigned int rng;
} LongoWorld;

/* Iterate live instances of an object class (includes child classes). */
int longo_is_block_family(int object);
int longo_instance_exists(const LongoWorld *w, int object);
int longo_instance_number(const LongoWorld *w, int object);
LongoInst *longo_find_first(const LongoWorld *w, int object);
LongoInst *longo_find_id(const LongoWorld *w, int id);

void longo_init(LongoWorld *w, unsigned int seed);
void longo_room_goto(LongoWorld *w, int room_index);
void longo_room_goto_next(LongoWorld *w);
void longo_room_restart(LongoWorld *w);

/* Advance the simulation by one 60 Hz GameMaker frame. */
void longo_tick(LongoWorld *w, const LongoInput *input, double delta_ms);

/* Sound events generated during the last tick (poll then clear). */
int longo_poll_sound(LongoWorld *w, LongoSoundEvent *out);

/* Sprite metadata (renderer mirrors GameMaker origin/frames semantics). */
int longo_sprite_width(int sprite);
int longo_sprite_height(int sprite);
int longo_sprite_origin_x(int sprite);
int longo_sprite_origin_y(int sprite);
int longo_sprite_frames(int sprite);

/* Wave() global script, exposed for the renderer's visual-only uses. */
float longo_wave(float a, float b, float period, float phase, double time_ms);

#endif /* LONGO_DOGGO_GAME_H */
