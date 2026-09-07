#include "level_data.h"

#include <stddef.h>

/*
 * Room data policy: this is the ONE translation unit that owns the
 * authored room tables and the catalog.  The tables stay plain readable
 * C arrays (no serialization, no generated format); every other file
 * reaches them through longo_room()/longo_room_count(), so no includer
 * compiles its own copy.  The catalog lists the rooms in play order —
 * index 0 is the start room, sim_room_goto_next() walks the leading
 * in-play run, and the two trailing authored rooms (editor, levelbase)
 * stay out of play.  Each room carries its LongoRoomId: the tile maps
 * (room_tiles.c) key off that id, and the tutorial dialogue selection
 * (objects/dialogue.c) keys off the same catalog play indices as
 * authored content.
 */

static const LongoRoomObject longo_room_title_screen_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, -300 },
    { LONGO_OBJ_DOG, 56, 72, 1, 1, -400 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 20 },
    { LONGO_OBJ_GOAL, 152, 64, 1, 1, -400 },
    { LONGO_OBJ_TRANSITION, 128, 0, 1, 1, -600 },
    { LONGO_OBJ_WIN, 136, 64, 2, 1, -400 },
    { LONGO_OBJ_TITLE, 56, 16, 1, 1, -600 },
    { LONGO_OBJ_BUTTERFLY, 56, 96, 1, 1, -500 },
    { LONGO_OBJ_BUTTERFLY, 256, 96, 1, 1, -500 },
    { LONGO_OBJ_BUTTERFLY, 224, 160, 1, 1, -500 },
    { LONGO_OBJ_BOX, 256, 144, 1, 1, -500 },
    { LONGO_OBJ_BOX, 224, 64, 1, 1, -500 },
    { LONGO_OBJ_APPLE, 48, 160, 1, 1, -500 },
    { LONGO_OBJ_APPLE, 32, 144, 1, 1, -500 },
    { LONGO_OBJ_BUTTON, 144, 104, 1, 1, -500 },
    { LONGO_OBJ_FLOWER, 256, 176, 1, 1, -500 },
    { LONGO_OBJ_FLOWER, 264, 120, 1, 1, -500 },
    { LONGO_OBJ_FLOWER, 72, 168, 1, 1, -500 },
    { LONGO_OBJ_FLOWER, 8, 96, 1, 1, -500 },
    { LONGO_OBJ_DOOR, 256, 32, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 240, 16, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 256, 16, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 272, 16, 1, 1, -500 },
    { LONGO_OBJ_BOX, 32, 128, 1, 1, -500 },
    { LONGO_OBJ_BOX, 16, 112, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 240, 0, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 256, 0, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 272, 0, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 288, 128, 1, 1, -500 },
    { LONGO_OBJ_PEAR, 32, 32, 1, 1, -500 },
    { LONGO_OBJ_APPLE, 80, 16, 1, 1, -500 },
    { LONGO_OBJ_APPLE, 240, 96, 1, 1, -500 },
    { LONGO_OBJ_FLOWER, 80, 48, 1, 1, -500 },
    { LONGO_OBJ_FLOWER, 96, 40, 1, 1, -500 },
};

static const LongoRoom longo_room_title_screen = {
    "rm_title_screen", LONGO_ROOM_ID_TITLE_SCREEN, 304, 208,
    longo_room_title_screen_objects,
    (int)(sizeof(longo_room_title_screen_objects) / sizeof(longo_room_title_screen_objects[0]))
};

static const LongoRoomObject longo_room_tutorial_objects[] = {
    { LONGO_OBJ_DOG, 40, 56, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 0, 0, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 208, 160, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 96, 160, 1, 1, 300 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 500 },
    { LONGO_OBJ_GOAL, 232, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 0, 0, 19, 1, 100 },
    { LONGO_OBJ_BLOCK, 0, 192, 19, 1, 100 },
    { LONGO_OBJ_BLOCK, 288, 8, 1, 12.5, 100 },
    { LONGO_OBJ_BLOCK, 0, 0, 1, 12.5, 100 },
    { LONGO_OBJ_PEAR, 64, 160, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 80, 144, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 176, 160, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 192, 144, 1, 1, 300 },
    { LONGO_OBJ_TUTORIAL, 408, 88, 0.6666667, 0.6666667, 0 },
    { LONGO_OBJ_WIN, 216, 80, 2, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 32, 112, 1, 1, 100 },
    { LONGO_OBJ_BUTTERFLY, 136, 72, 1, 1, 100 },
    { LONGO_OBJ_FLOWER, 232, 136, 1, 1, 400 },
    { LONGO_OBJ_FLOWER, 280, 56, 1, 1, 400 },
    { LONGO_OBJ_FLOWER, 256, 160, 1, 1, 400 },
    { LONGO_OBJ_FLOWER, 112, 152, 1, 1, 400 },
    { LONGO_OBJ_BLOCK, 96, 48, 1, 1, 100 },
    { LONGO_OBJ_BLOCK, 160, 96, 1, 1, 100 },
    { LONGO_OBJ_APPLE, 328, 96, 1, 1, 0 },
};

static const LongoRoom longo_room_tutorial = {
    "rm_tutorial", LONGO_ROOM_ID_TUTORIAL, 304, 208,
    longo_room_tutorial_objects,
    (int)(sizeof(longo_room_tutorial_objects) / sizeof(longo_room_tutorial_objects[0]))
};

static const LongoRoomObject longo_room_level6_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 100 },
    { LONGO_OBJ_DOG, 24, 88, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, 208, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 1, 15, 0 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 176, 0, 1, 9, 0 },
    { LONGO_OBJ_BLOCK, 176, 80, 4, 1, 0 },
    { LONGO_OBJ_HOUSESPAWNER, 240, 32, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 256, 80, 3, 1, 0 },
    { LONGO_OBJ_DOOR, 240, 80, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 224, 144, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 240, 128, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 272, 64, 1, 1, 0 },
    { LONGO_OBJ_BOX, 80, 32, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 32, 32, 1, 1, 0 },
    { LONGO_OBJ_BOX, 96, 96, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 128, 32, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 192, 0, 1, 3, 0 },
    { LONGO_OBJ_BLOCK, 288, 0, 1, 3, 0 },
    { LONGO_OBJ_BLOCK, 272, 0, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 208, 0, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 192, 48, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 208, 64, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 288, 48, 1, 1, 0 },
    { LONGO_OBJ_BOX, 0, 192, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 264, 168, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 64, 64, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 232, 176, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 272, 184, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 56, 112, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 168, 40, 1, 1, 0 },
    { LONGO_OBJ_BOX, 208, 16, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 160, 120, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 208, 160, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 176, 144, 1, 1, 0 },
    { LONGO_OBJ_TUTORIAL, 416, 96, 1, 1, -200 },
    { LONGO_OBJ_BLOCK, 176, 160, 1, 9, 0 },
    { LONGO_OBJ_BLOCK, 272, 128, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 288, 192, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 208, 176, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 384, 160, 1, 1, 0 },
};

static const LongoRoom longo_room_level6 = {
    "rm_level6", LONGO_ROOM_ID_LEVEL6, 304, 208,
    longo_room_level6_objects,
    (int)(sizeof(longo_room_level6_objects) / sizeof(longo_room_level6_objects[0]))
};

static const LongoRoomObject longo_room_level5_objects[] = {
    { LONGO_OBJ_DOG, 24, 104, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 0, 0, 1, 1, 300 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 400 },
    { LONGO_OBJ_BLOCK, 0, 0, 19, 1, 100 },
    { LONGO_OBJ_BLOCK, 0, 192, 19, 1, 100 },
    { LONGO_OBJ_BLOCK, 288, 8, 1, 12.5, 100 },
    { LONGO_OBJ_BLOCK, 0, 0, 1, 12.5, 100 },
    { LONGO_OBJ_BUTTERFLY, 120, 152, 1, 1, 100 },
    { LONGO_OBJ_BUTTERFLY, 256, 128, 1, 1, 100 },
    { LONGO_OBJ_BLOCK, 16, 48, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 32, 144, 2, 1, 300 },
    { LONGO_OBJ_BLOCK, 64, 160, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 96, 160, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 112, 144, 1, 3, 300 },
    { LONGO_OBJ_BLOCK, 112, 112, 3, 1, 300 },
    { LONGO_OBJ_BLOCK, 144, 128, 1, 3, 300 },
    { LONGO_OBJ_BLOCK, 176, 176, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 176, 144, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 160, 160, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 192, 160, 2, 1, 300 },
    { LONGO_OBJ_BLOCK, 224, 112, 1, 5, 300 },
    { LONGO_OBJ_BLOCK, 224, 16, 1, 5, 300 },
    { LONGO_OBJ_DOOR, 224, 96, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 208, 112, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 208, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 160, 96, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 160, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 192, 16, 1, 2, 300 },
    { LONGO_OBJ_BUTTON, 160, 32, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 128, 48, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 112, 16, 1, 2, 300 },
    { LONGO_OBJ_BLOCK, 96, 16, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 80, 32, 1, 2, 300 },
    { LONGO_OBJ_BLOCK, 96, 80, 1, 3, 300 },
    { LONGO_OBJ_BLOCK, 112, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 80, 112, 1, 2, 300 },
    { LONGO_OBJ_BOX, 64, 96, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 48, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 48, 16, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 208, 128, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 256, 160, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 208, 96, 1, 1, 300 },
    { LONGO_OBJ_HOUSESPAWNER, 256, 48, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 208, 16, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 272, 176, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 272, 120, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 192, 176, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 216, 184, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 136, 136, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 112, 104, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 104, 40, 1, 1, 300 },
    { LONGO_OBJ_DOOR, 176, 96, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 192, 96, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 56, 32, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 328, 112, 1, 1, 0 },
};

static const LongoRoom longo_room_level5 = {
    "rm_level5", LONGO_ROOM_ID_LEVEL5, 304, 208,
    longo_room_level5_objects,
    (int)(sizeof(longo_room_level5_objects) / sizeof(longo_room_level5_objects[0]))
};

static const LongoRoomObject longo_room_level4_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 100 },
    { LONGO_OBJ_DOG, 40, 72, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, 208, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 1, 15, 0 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 300 },
    { LONGO_OBJ_BOX, 16, 48, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 48, 0, 1, 4, 0 },
    { LONGO_OBJ_BLOCK, 48, 48, 5, 1, 0 },
    { LONGO_OBJ_BLOCK, 48, 80, 5, 1, 0 },
    { LONGO_OBJ_HOLE, 112, 64, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 48, 112, 5, 1, 0 },
    { LONGO_OBJ_BLOCK, 48, 112, 1, 6, 0 },
    { LONGO_OBJ_DOOR, 48, 96, 1, 1, 0 },
    { LONGO_OBJ_DOOR, 16, 160, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 0, 160, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 32, 160, 3, 1, 0 },
    { LONGO_OBJ_PEAR, 0, 176, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 16, 192, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 32, 176, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 64, 176, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 80, 192, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 96, 176, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 96, 160, 2, 1, 0 },
    { LONGO_OBJ_BLOCK, 112, 160, 1, 3, 0 },
    { LONGO_OBJ_HOLE, 80, 160, 1, 1, 0 },
    { LONGO_OBJ_BOX, 80, 16, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 64, 96, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 80, 96, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 96, 96, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 112, 96, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 192, 96, 1, 1, 0 },
    { LONGO_OBJ_HOUSESPAWNER, 224, 64, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 272, 144, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 96, 16, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 80, 136, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 240, 184, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 264, 16, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 64, 192, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 8, 128, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 160, 48, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 208, 16, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 272, 192, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 64, 128, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 96, 64, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 16, 176, 1, 1, 0 },
    { LONGO_OBJ_TUTORIAL, 368, 88, 1, 1, -200 },
    { LONGO_OBJ_APPLE, 360, 152, 1, 1, -200 },
};

static const LongoRoom longo_room_level4 = {
    "rm_level4", LONGO_ROOM_ID_LEVEL4, 304, 208,
    longo_room_level4_objects,
    (int)(sizeof(longo_room_level4_objects) / sizeof(longo_room_level4_objects[0]))
};

static const LongoRoomObject longo_room_level2_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 400 },
    { LONGO_OBJ_DOG, 24, 88, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 64, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 300 },
    { LONGO_OBJ_BLOCK, -16, 0, 21, 1, 300 },
    { LONGO_OBJ_BLOCK, -8, 192, 21, 1, 300 },
    { LONGO_OBJ_BLOCK, 0, -8, 1, 15, 300 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 500 },
    { LONGO_OBJ_BLOCK, 32, 32, 16, 1, 300 },
    { LONGO_OBJ_APPLE, 32, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 288, 96, 1, 7.5, 300 },
    { LONGO_OBJ_BLOCK, 160, 96, 8.5, 1, 300 },
    { LONGO_OBJ_BLOCK, 224, 32, 1, 7, 300 },
    { LONGO_OBJ_BLOCK, 32, 48, 1, 1, 300 },
    { LONGO_OBJ_BOX, 32, 80, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 16, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 32, 96, 1, 5, 300 },
    { LONGO_OBJ_BLOCK, 40, 160, 6.4999995, 1, 300 },
    { LONGO_OBJ_BLOCK, 192, 128, 1, 2, 300 },
    { LONGO_OBJ_BLOCK, 208, 160, 4, 1, 300 },
    { LONGO_OBJ_BLOCK, 256, 128, 1, 2, 300 },
    { LONGO_OBJ_HOUSESPAWNER, 256, 64, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 192, 176, 1, 1, 300 },
    { LONGO_OBJ_BOX, 208, 176, 1, 1, 300 },
    { LONGO_OBJ_DOOR, 144, 160, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 160, 112, 1, 4, 300 },
    { LONGO_OBJ_BLOCK, 48, 128, 3, 1, 300 },
    { LONGO_OBJ_APPLE, 48, 144, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 64, 144, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 80, 144, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 112, 128, 2, 1, 300 },
    { LONGO_OBJ_BLOCK, 128, 80, 1, 3.5, 300 },
    { LONGO_OBJ_BLOCK, 64, 96, 2, 1, 300 },
    { LONGO_OBJ_BLOCK, 80, 40, 1, 3.5, 300 },
    { LONGO_OBJ_BLOCK, 96, 48, 3, 1, 300 },
    { LONGO_OBJ_BLOCK, 160, 48, 4, 1, 300 },
    { LONGO_OBJ_BOX, 144, 64, 1, 1, 300 },
    { LONGO_OBJ_BOX, 160, 64, 1, 1, 300 },
    { LONGO_OBJ_BOX, 192, 64, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 176, 64, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 160, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 96, 80, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 96, 64, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 96, 96, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 192, 160, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 240, 88, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 184, 136, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 128, 144, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 0, 112, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 168, 24, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 224, 144, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 64, 112, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 352, 104, 1, 1, 0 },
};

static const LongoRoom longo_room_level2 = {
    "rm_level2", LONGO_ROOM_ID_LEVEL2, 304, 208,
    longo_room_level2_objects,
    (int)(sizeof(longo_room_level2_objects) / sizeof(longo_room_level2_objects[0]))
};

static const LongoRoomObject longo_room_level1_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 400 },
    { LONGO_OBJ_HOUSESPAWNER, 64, 48, 1, 1, 300 },
    { LONGO_OBJ_DOG, 72, 88, 1, 1, 300 },
    { LONGO_OBJ_BOX, 96, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 160, 48, 5, 1, 300 },
    { LONGO_OBJ_BLOCK, 208, 16, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 240, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 272, 16, 1, 8, 300 },
    { LONGO_OBJ_BLOCK, 160, 0, 8, 1, 300 },
    { LONGO_OBJ_HOLE, 256, 80, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 240, 80, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 224, 80, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 208, 80, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 192, 64, 1, 4, 300 },
    { LONGO_OBJ_BLOCK, 256, 112, 1, 3, 300 },
    { LONGO_OBJ_DOOR, 240, 112, 1, 1, 300 },
    { LONGO_OBJ_DOOR, 224, 112, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 256, 96, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 240, 96, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 224, 96, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 208, 128, 1, 2, 300 },
    { LONGO_OBJ_BUTTON, 192, 128, 1, 1, 300 },
    { LONGO_OBJ_BOX, 224, 128, 1, 1, 300 },
    { LONGO_OBJ_BOX, 240, 128, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 224, 160, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 240, 160, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 224, 144, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 240, 144, 1, 1, 300 },
    { LONGO_OBJ_BOX, 256, 160, 1, 1, 300 },
    { LONGO_OBJ_BOX, 208, 160, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 224, 64, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 192, 144, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 288, 128, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 300 },
    { LONGO_OBJ_BLOCK, -16, -16, 21, 1, 300 },
    { LONGO_OBJ_BLOCK, -16, 208, 21, 1, 300 },
    { LONGO_OBJ_BLOCK, -16, -16, 1, 15, 300 },
    { LONGO_OBJ_BUTTON, 256, 64, 1, 1, 300 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 500 },
    { LONGO_OBJ_BLOCK, 192, 16, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 176, 16, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 128, 136, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 24, 16, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 168, 184, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 32, 176, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 288, 16, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 296, 24, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 288, 32, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 296, 40, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 40, 96, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 272, 160, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 272, 144, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 288, 144, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 288, 200, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 192, 32, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 344, 120, 1, 1, 300 },
};

static const LongoRoom longo_room_level1 = {
    "rm_level1", LONGO_ROOM_ID_LEVEL1, 304, 208,
    longo_room_level1_objects,
    (int)(sizeof(longo_room_level1_objects) / sizeof(longo_room_level1_objects[0]))
};

static const LongoRoomObject longo_room_level3_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 400 },
    { LONGO_OBJ_DOG, 8, 40, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 300 },
    { LONGO_OBJ_BLOCK, -16, -16, 21, 1, 300 },
    { LONGO_OBJ_BLOCK, -16, 208, 21, 1, 300 },
    { LONGO_OBJ_BLOCK, -16, -16, 1, 15, 300 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 500 },
    { LONGO_OBJ_BLOCK, 16, 16, 1, 10, 300 },
    { LONGO_OBJ_HOLE, 0, 176, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 0, 160, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 0, 144, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 0, 128, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 0, 16, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 0, 0, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 16, 0, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 32, 0, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 16, 48, 3, 1, 300 },
    { LONGO_OBJ_BLOCK, 32, 80, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 32, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 32, 160, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 64, 48, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 80, 48, 3, 1, 300 },
    { LONGO_OBJ_BOX, 80, 64, 1, 1, 300 },
    { LONGO_OBJ_BOX, 80, 80, 1, 1, 300 },
    { LONGO_OBJ_DOOR, 112, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 112, 80, 1, 5, 300 },
    { LONGO_OBJ_BLOCK, 112, 176, 2, 1, 300 },
    { LONGO_OBJ_BLOCK, 160, 176, 2, 1, 300 },
    { LONGO_OBJ_DOOR, 144, 176, 1, 1, 300 },
    { LONGO_OBJ_HOUSESPAWNER, 144, 144, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 176, 80, 1, 5, 300 },
    { LONGO_OBJ_BOX, 128, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 128, 32, 3, 1, 300 },
    { LONGO_OBJ_BLOCK, 176, 48, 3, 1, 300 },
    { LONGO_OBJ_DOOR, 176, 64, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 144, 192, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 272, 16, 1, 10, 300 },
    { LONGO_OBJ_BLOCK, 256, 160, 2, 1, 300 },
    { LONGO_OBJ_BUTTON, 112, 32, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 144, 0, 1, 3, 300 },
    { LONGO_OBJ_BLOCK, 128, 96, 3, 1, 300 },
    { LONGO_OBJ_BOX, 208, 64, 1, 1, 300 },
    { LONGO_OBJ_BOX, 208, 80, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 256, 64, 1, 1, 300 },
    { LONGO_OBJ_BLOCK, 240, 48, 3, 1, 300 },
    { LONGO_OBJ_BLOCK, 256, 80, 2, 1, 300 },
    { LONGO_OBJ_BUTTON, 288, 48, 1, 1, 300 },
    { LONGO_OBJ_DOOR, 288, 16, 1, 1, 300 },
    { LONGO_OBJ_BUTTON, 288, 192, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 144, 64, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 160, 80, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 160, 48, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 128, 80, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 128, 48, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 32, 80, 1, 1, 300 },
    { LONGO_OBJ_BUTTERFLY, 240, 16, 1, 1, 300 },
    { LONGO_OBJ_BOX, 80, 16, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 240, 64, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 72, 120, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 240, 184, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 200, 8, 1, 1, 300 },
    { LONGO_OBJ_FLOWER, 8, 112, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 176, 160, 1, 1, 300 },
    { LONGO_OBJ_HOLE, 112, 160, 1, 1, 300 },
    { LONGO_OBJ_APPLE, 352, 104, 1, 1, 0 },
};

static const LongoRoom longo_room_level3 = {
    "rm_level3", LONGO_ROOM_ID_LEVEL3, 304, 208,
    longo_room_level3_objects,
    (int)(sizeof(longo_room_level3_objects) / sizeof(longo_room_level3_objects[0]))
};

static const LongoRoomObject longo_room_credits_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 100 },
    { LONGO_OBJ_DOG, 120, 88, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 0 },
    { LONGO_OBJ_BLOCK, -16, 0, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, 208, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 1, 15, 0 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 300 },
    { LONGO_OBJ_PEAR, 224, 96, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 64, 48, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 48, 160, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 272, 144, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 240, 32, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 32, 80, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 144, 176, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 256, 128, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 256, 96, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 256, 64, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 272, 112, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 240, 112, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 240, 144, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 96, 128, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 96, 64, 1, 1, 0 },
    { LONGO_OBJ_BOX, 64, 96, 1, 1, 0 },
    { LONGO_OBJ_BOX, 208, 64, 1, 1, 0 },
    { LONGO_OBJ_BOX, 32, 48, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 256, 144, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 24, 128, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 168, 48, 1, 1, 0 },
    { LONGO_OBJ_BUTTERFLY, 24, 72, 1, 1, 0 },
    { LONGO_OBJ_HOLE, 0, 176, 1, 1, 0 },
    { LONGO_OBJ_HOLE, 96, 192, 1, 1, 0 },
    { LONGO_OBJ_BOX, 176, 128, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 80, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 16, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 32, 176, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 48, 192, 1, 1, 0 },
    { LONGO_OBJ_HOLE, 0, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 64, 176, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 208, 128, 1, 1, 0 },
    { LONGO_OBJ_BUTTON, 16, 64, 1, 1, 0 },
    { LONGO_OBJ_DOOR, 208, 160, 1, 1, 0 },
    { LONGO_OBJ_BOX, 128, 160, 1, 1, 0 },
    { LONGO_OBJ_BOX, 176, 176, 1, 1, 0 },
    { LONGO_OBJ_BOX, 144, 160, 1, 1, 0 },
    { LONGO_OBJ_BOX, 160, 176, 1, 1, 0 },
    { LONGO_OBJ_BOX, 176, 160, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 224, 160, 5, 1, 0 },
    { LONGO_OBJ_BOX, 160, 160, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 192, 160, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 256, 48, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 32, 112, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 192, 176, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 272, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 240, 192, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 144, 144, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 128, 176, 1, 1, 0 },
    { LONGO_OBJ_DOOR, 128, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 256, 176, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 208, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 144, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 208, 176, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 192, 192, 1, 1, 0 },
    { LONGO_OBJ_PEAR, 176, 192, 1, 1, 0 },
    { LONGO_OBJ_APPLE, 224, 192, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 96, 32, 1, 1, 0 },
    { LONGO_OBJ_FLOWER, 280, 80, 1, 1, -200 },
    { LONGO_OBJ_FLOWER, 16, 24, 1, 1, -200 },
    { LONGO_OBJ_FLOWER, 56, 152, 1, 1, -200 },
    { LONGO_OBJ_FLOWER, 112, 184, 1, 1, -200 },
    { LONGO_OBJ_FLOWER, 176, 72, 1, 1, -200 },
    { LONGO_OBJ_FLOWER, 256, 112, 1, 1, -200 },
    { LONGO_OBJ_TUTORIAL, 488, 72, 1, 1, -200 },
    { LONGO_OBJ_APPLE, 344, 136, 1, 1, 0 },
};

static const LongoRoom longo_room_credits = {
    "rm_credits", LONGO_ROOM_ID_CREDITS, 304, 208,
    longo_room_credits_objects,
    (int)(sizeof(longo_room_credits_objects) / sizeof(longo_room_credits_objects[0]))
};

static const LongoRoomObject longo_room_editor_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, -54 },
    { LONGO_OBJ_MOUSE, 128, 96, 1, 1, -400 },
};

static const LongoRoom longo_room_editor = {
    "rm_editor", LONGO_ROOM_ID_EDITOR, 304, 208,
    longo_room_editor_objects,
    (int)(sizeof(longo_room_editor_objects) / sizeof(longo_room_editor_objects[0]))
};

static const LongoRoomObject longo_room_levelbase_objects[] = {
    { LONGO_OBJ_FLOWER, 0, 224, 1, 1, 100 },
    { LONGO_OBJ_DOG, 24, 88, 1, 1, 0 },
    { LONGO_OBJ_BLOCK, 304, -16, 1, 15, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, 208, 21, 1, 0 },
    { LONGO_OBJ_BLOCK, -16, -16, 1, 15, 0 },
    { LONGO_OBJ_SHADOWS, 0, 0, 1, 1, 300 },
};

static const LongoRoom longo_room_levelbase = {
    "rm_levelbase", LONGO_ROOM_ID_LEVELBASE, 304, 208,
    longo_room_levelbase_objects,
    (int)(sizeof(longo_room_levelbase_objects) / sizeof(longo_room_levelbase_objects[0]))
};

/* The one catalog: play order is array order and in_play marks the
 * shipped runtime prefix (the play bound behind longo_room_play_count(),
 * which sim_room_goto rejects indices past).  Ids are the identity the
 * rest of the game keys off; the names here are the authored
 * GameMaker room names. */
typedef struct LongoRoomSlot {
    const LongoRoom *room;
    int in_play;
} LongoRoomSlot;

static const LongoRoomSlot longo_catalog[LONGO_ROOM_ID_COUNT] = {
    { &longo_room_title_screen, 1 },
    { &longo_room_tutorial, 1 },
    { &longo_room_level6, 1 },
    { &longo_room_level5, 1 },
    { &longo_room_level4, 1 },
    { &longo_room_level2, 1 },
    { &longo_room_level1, 1 },
    { &longo_room_level3, 1 },
    { &longo_room_credits, 1 },
    { &longo_room_editor, 0 },
    { &longo_room_levelbase, 0 }
};

int longo_room_count(void) { return LONGO_ROOM_ID_COUNT; }

const LongoRoom *longo_room(int index)
{
    if (index < 0 || index >= LONGO_ROOM_ID_COUNT) return NULL;
    return longo_catalog[index].room;
}

/* The in-play rooms are an authored prefix: the count is the length of
 * that run, so an out-of-play room can never be reached by walking. */
int longo_room_play_count(void)
{
    int n = 0;
    while (n < LONGO_ROOM_ID_COUNT && longo_catalog[n].in_play) n++;
    return n;
}
