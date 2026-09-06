#ifndef LONGO_DOGGO_ROOM_TILES_H
#define LONGO_DOGGO_ROOM_TILES_H

#include <stddef.h>

#define LONGO_ROOM_TILE_WIDTH 38
#define LONGO_ROOM_TILE_HEIGHT 26
#define LONGO_ROOM_TILE_COUNT 11

enum {
    LONGO_ROOM_TUTORIAL_INDEX = 0,
    LONGO_ROOM_LEVEL1_INDEX = 1,
    LONGO_ROOM_LEVEL2_INDEX = 2,
    LONGO_ROOM_LEVEL3_INDEX = 3,
    LONGO_ROOM_LEVEL4_INDEX = 4,
    LONGO_ROOM_LEVEL5_INDEX = 5,
    LONGO_ROOM_LEVEL6_INDEX = 6,
    LONGO_ROOM_TITLE_SCREEN_INDEX = 7,
    LONGO_ROOM_CREDITS_INDEX = 8,
    LONGO_ROOM_EDITOR_INDEX = 9,
    LONGO_ROOM_LEVELBASE_INDEX = 10,
};

typedef enum LongoTileSet {
    LONGO_TILESET_TILESET1 = 0
} LongoTileSet;

typedef struct LongoTileLayer {
    const unsigned int *data;
    int width;
    int height;
    LongoTileSet tileset;
    int depth;
    int offset_x;
    int offset_y;
} LongoTileLayer;

typedef struct LongoRoomTileMap {
    const char *room_name;
    LongoTileLayer tiles_3;
    int background_depth;
    int shadows_depth;
    int flowers_depth;
    int objects_depth;
    int blocks_depth;
    int gui_depth;
} LongoRoomTileMap;

extern const LongoRoomTileMap longo_room_tile_maps[LONGO_ROOM_TILE_COUNT];

/* Tile map for a room by name (NULL when the room has none). */
struct LongoRoom;
const LongoRoomTileMap *room_tiles_for(const struct LongoRoom *room);

#endif
