#ifndef LONGO_DOGGO_ROOM_TILES_H
#define LONGO_DOGGO_ROOM_TILES_H

#include <stddef.h>

#include "level_data.h"

#define LONGO_ROOM_TILE_WIDTH 38
#define LONGO_ROOM_TILE_HEIGHT 26

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
    int room_id; /* LongoRoomId: the catalog entry these tiles belong to */
    LongoTileLayer tiles_3;
    int background_depth;
    int shadows_depth;
    int flowers_depth;
    int objects_depth;
    int blocks_depth;
    int gui_depth;
} LongoRoomTileMap;

/* One tile map per authored room, indexed by LongoRoomId so the lookup
 * keys off the catalog entry instead of re-matching room names. */
extern const LongoRoomTileMap longo_room_tile_maps[LONGO_ROOM_ID_COUNT];

/* Tile map for a catalog room (NULL when the room has none). */
const LongoRoomTileMap *room_tiles_for(const struct LongoRoom *room);

#endif
