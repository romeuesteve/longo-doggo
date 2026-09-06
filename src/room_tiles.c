#include "room_tiles.h"

#include "level_data.h"

#include <string.h>

static const unsigned int longo_tutorial_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    6u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 0u, 
    14u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 0u, 
    12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 0u
};


static const unsigned int longo_level1_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 0u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 0u, 10u, 11u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 8u, 6u, 0u, 0u, 9u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 8u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 5u, 5u, 4u, 5u, 5u, 4u, 2u, 3u, 0u, 0u, 0u, 0u, 6u, 0u, 8u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 13u, 13u, 12u, 13u, 13u, 12u, 10u, 11u, 0u, 0u, 0u, 0u, 14u, 0u, 8u, 9u, 
    9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 6u, 0u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 14u, 0u, 10u, 11u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 5u, 4u, 5u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 13u, 12u, 13u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 9u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 4u, 0u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 12u, 0u, 10u, 11u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 10u, 11u, 
    0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};


static const unsigned int longo_level2_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 
    12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 
    2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    2u, 3u, 0u, 0u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 5u, 4u, 5u, 4u, 5u, 4u, 4u, 0u, 0u, 
    10u, 11u, 0u, 0u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 13u, 12u, 13u, 12u, 13u, 12u, 12u, 0u, 0u, 
    2u, 3u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 2u, 3u, 2u, 3u, 0u, 0u, 2u, 3u, 2u, 3u, 2u, 3u, 2u, 3u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    10u, 11u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 10u, 11u, 10u, 11u, 0u, 0u, 10u, 11u, 10u, 11u, 10u, 11u, 10u, 11u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 
    0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 6u, 0u, 0u, 6u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 6u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 
    0u, 14u, 0u, 0u, 14u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 14u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 
    0u, 6u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 
    0u, 14u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 
    0u, 6u, 0u, 0u, 6u, 5u, 4u, 5u, 4u, 5u, 4u, 4u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 6u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 2u, 3u, 0u, 0u, 6u, 0u, 0u, 0u, 2u, 3u, 
    0u, 14u, 0u, 0u, 14u, 13u, 12u, 13u, 12u, 13u, 12u, 12u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 14u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 10u, 11u, 0u, 0u, 14u, 0u, 0u, 0u, 10u, 11u, 
    0u, 6u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 2u, 3u, 
    0u, 14u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 10u, 11u, 
    9u, 6u, 0u, 0u, 4u, 5u, 4u, 4u, 2u, 3u, 2u, 3u, 2u, 3u, 2u, 3u, 2u, 3u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 0u, 0u, 0u, 2u, 3u, 
    0u, 14u, 0u, 0u, 12u, 13u, 12u, 12u, 10u, 11u, 10u, 11u, 10u, 11u, 10u, 11u, 10u, 11u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 0u, 0u, 0u, 10u, 11u, 
    8u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 
    0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 10u, 11u, 
    0u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 
    0u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u
};


static const unsigned int longo_level3_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 5u, 4u, 2u, 3u, 0u, 0u, 4u, 5u, 5u, 5u, 5u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 5u, 5u, 5u, 4u, 0u, 0u, 2u, 3u, 4u, 5u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 13u, 12u, 10u, 11u, 0u, 0u, 12u, 13u, 13u, 13u, 13u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 13u, 13u, 13u, 12u, 0u, 0u, 10u, 11u, 12u, 13u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 5u, 5u, 4u, 5u, 5u, 4u, 5u, 5u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 13u, 13u, 12u, 13u, 13u, 12u, 13u, 13u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 8u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 
    0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 
    0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 
    0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 
    0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 5u, 4u, 0u, 0u, 4u, 5u, 5u, 4u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 13u, 12u, 0u, 0u, 12u, 13u, 13u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};


static const unsigned int longo_level4_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 4u, 5u, 5u, 5u, 5u, 4u, 5u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 12u, 13u, 13u, 13u, 13u, 12u, 13u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 4u, 5u, 5u, 5u, 5u, 4u, 5u, 4u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 12u, 13u, 13u, 13u, 13u, 12u, 13u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 6u, 5u, 4u, 5u, 5u, 5u, 5u, 4u, 5u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 14u, 13u, 12u, 13u, 13u, 13u, 13u, 12u, 13u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    5u, 4u, 0u, 0u, 4u, 5u, 6u, 0u, 2u, 3u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    13u, 12u, 0u, 0u, 12u, 13u, 14u, 0u, 10u, 11u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u
};


static const unsigned int longo_level5_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    6u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 6u, 0u, 
    14u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 5u, 5u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 13u, 13u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 5u, 5u, 4u, 0u, 0u, 0u, 0u, 4u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 13u, 13u, 12u, 0u, 0u, 0u, 0u, 12u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 4u, 5u, 5u, 4u, 5u, 6u, 0u, 0u, 0u, 0u, 8u, 9u, 2u, 3u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 12u, 13u, 13u, 12u, 13u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 21u, 0u, 0u, 0u, 6u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 6u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 14u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 6u, 2u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 14u, 10u, 
    4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 0u, 
    12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 0u
};


static const unsigned int longo_level6_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 2u, 3u, 
    0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 10u, 11u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 5u, 4u, 5u, 4u, 5u, 4u, 0u, 0u, 4u, 5u, 4u, 5u, 4u, 5u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 13u, 12u, 13u, 12u, 13u, 12u, 0u, 0u, 12u, 13u, 12u, 13u, 12u, 13u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 
    0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 14u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u
};


static const unsigned int longo_title_screen_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    2u, 3u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    10u, 11u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 4u, 5u, 4u, 0u, 0u, 4u, 5u, 4u, 5u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12u, 13u, 12u, 13u, 12u, 0u, 0u, 12u, 13u, 12u, 13u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};


static const unsigned int longo_credits_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 
    12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 0u, 
    0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 8u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 11u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 9u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 9u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};


static const unsigned int longo_editor_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 6u, 0u, 
    14u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 14u, 0u, 
    4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 5u, 4u, 0u, 
    12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 13u, 12u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};


static const unsigned int longo_levelbase_tiles_3[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};


const LongoRoomTileMap longo_room_tile_maps[LONGO_ROOM_TILE_COUNT] = {
    { "rm_tutorial", { longo_tutorial_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 700, 500, 400, 300, 100, 0 },
    { "rm_level1", { longo_level1_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 700, 500, 400, 300, 100, 0 },
    { "rm_level2", { longo_level2_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 700, 500, 400, 300, 100, 0 },
    { "rm_level3", { longo_level3_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 700, 500, 400, 300, 0, 0 },
    { "rm_level4", { longo_level4_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 500, 300, 100, 0, -100, -200 },
    { "rm_level5", { longo_level5_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 600, 400, 0, 300, 100, 0 },
    { "rm_level6", { longo_level6_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 500, 300, 100, 0, -100, -200 },
    { "rm_title_screen", { longo_title_screen_tiles_3, 38, 26, LONGO_TILESET_TILESET1, -200, 0, 0 }, 220, 20, -300, -400, -500, -600 },
    { "rm_credits", { longo_credits_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 500, 300, 100, 0, -100, -200 },
    { "rm_editor", { longo_editor_tiles_3, 38, 26, LONGO_TILESET_TILESET1, -200, 0, 0 }, 220, 20, -54, -127, -300, -400 },
    { "rm_levelbase", { longo_levelbase_tiles_3, 38, 26, LONGO_TILESET_TILESET1, 200, 0, 0 }, 500, 300, 100, 0, -100, -200 },
};

const LongoRoomTileMap *room_tiles_for(const struct LongoRoom *room)
{
    if (room == NULL) return NULL;
    for (int i = 0; i < LONGO_ROOM_TILE_COUNT; i++) {
        if (strcmp(longo_room_tile_maps[i].room_name, room->name) == 0)
            return &longo_room_tile_maps[i];
    }
    return NULL;
}
