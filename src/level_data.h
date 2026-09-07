/*
 * Room data types and the catalog accessors.  The authored tables
 * themselves live in level_data.c as plain readable C arrays (no
 * serialization, no generated format): that one translation unit owns
 * every byte of room data, so includers compile the types only and
 * reach the rooms through longo_room().
 */
#ifndef LONGO_DOGGO_LEVEL_DATA_H
#define LONGO_DOGGO_LEVEL_DATA_H

/* Object type ids; the numeric order matches the exported asset table. */
enum LongoObj {
    LONGO_OBJ_PEAR = 1,
    LONGO_OBJ_TUTORIAL = 2,
    LONGO_OBJ_BUTTERFLY = 3,
    LONGO_OBJ_BLOCK = 4,
    LONGO_OBJ_BUTTON = 5,
    LONGO_OBJ_HOUSESPAWNER = 6,
    LONGO_OBJ_APPLE = 7,
    LONGO_OBJ_HIDDEN_BLOCK = 8,
    LONGO_OBJ_TRANSITION = 9,
    LONGO_OBJ_HOLE = 10,
    LONGO_OBJ_MOUSE = 11,
    LONGO_OBJ_TITLE = 12,
    LONGO_OBJ_GOALUP = 13,
    LONGO_PAR_MODULE = 14,
    LONGO_OBJ_WIN = 15,
    LONGO_OBJ_GOAL = 16,
    LONGO_OBJ_POSTEFFECTS = 17,
    LONGO_OBJ_FLOWER = 18,
    LONGO_OBJ_DOOR = 19,
    LONGO_OBJ_DOGPART = 20,
    LONGO_OBJ_SMOKE = 21,
    LONGO_OBJ_BARK = 22,
    LONGO_OBJ_DOGSPAWNER = 23,
    LONGO_OBJ_BOX = 24,
    LONGO_OBJ_SHADOWS = 25,
    LONGO_OBJ_DOG = 26,
    LONGO_OBJ_ONE = 27
};

/* Authored room identity: one enumerator per room table, independent of
 * play order.  Every LongoRoom carries its id, and every consumer that
 * needs to find a room's other data (the tile maps in room_tiles.c)
 * keys off it instead of re-matching by name. */
typedef enum LongoRoomId {
    LONGO_ROOM_ID_TITLE_SCREEN = 0,
    LONGO_ROOM_ID_TUTORIAL,
    LONGO_ROOM_ID_LEVEL1,
    LONGO_ROOM_ID_LEVEL2,
    LONGO_ROOM_ID_LEVEL3,
    LONGO_ROOM_ID_LEVEL4,
    LONGO_ROOM_ID_LEVEL5,
    LONGO_ROOM_ID_LEVEL6,
    LONGO_ROOM_ID_CREDITS,
    LONGO_ROOM_ID_EDITOR,
    LONGO_ROOM_ID_LEVELBASE,
    LONGO_ROOM_ID_COUNT
} LongoRoomId;

typedef struct LongoRoomObject {
    unsigned char object;
    float x;
    float y;
    float xscale;
    float yscale;
    int depth; /* the draw depth it was placed with */
} LongoRoomObject;

typedef struct LongoRoom {
    const char *name;
    int id; /* LongoRoomId: the room's identity, not its play position */
    int width;
    int height;
    const LongoRoomObject *objects;
    int object_count;
} LongoRoom;

/* The catalog (level_data.c) lists the rooms in play order: index 0 is
 * the start room and sim_room_goto_next() walks the leading in-play run
 * of the catalog.  The two trailing authored rooms (editor, levelbase)
 * stay out of play. */
int longo_room_count(void);      /* authored rooms, in and out of play */
const LongoRoom *longo_room(int index); /* NULL when out of range */
int longo_room_play_count(void); /* the shipped runtime prefix */

#endif /* LONGO_DOGGO_LEVEL_DATA_H */
