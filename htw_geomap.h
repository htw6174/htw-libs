#ifndef HTW_GEOMAP_H_INCLUDED
#define HTW_GEOMAP_H_INCLUDED

#include <stdint.h>
#include "htw_core.h"

#define HTW_GEO_TILE_NAME_MAX_LENGTH 256

typedef struct htw_geo_MapTile {
    int id;
    void *content;
} htw_geo_MapTile;

void *htw_geo_loadTileDefinitions (char *path);
htw_geo_MapTile *htw_createTile (int id); // FIXME: no reason to allocate one map tile at a time. Remove when reworking tilemap system
char *htw_geo_getTileName (int id);

typedef struct {
    int x;
    int y;
} int2d;

/** Corresponds to the locations below on a square grid:
 * 6 1 -
 * 5 - 2
 * - 4 3
 */
enum hexDirection {
    HEX_DIRECTION_NORTH_EAST = 0, // 0, 1
    HEX_DIRECTION_EAST, // 1, 0
    HEX_DIRECTION_SOUTH_EAST, // 1, -1
    HEX_DIRECTION_SOUTH_WEST, // 0, -1
    HEX_DIRECTION_WEST, // -1, 0
    HEX_DIRECTION_NORTH_WEST, // -1, 1
    HEX_DIRECTION_COUNT // Limit for iterating over values in this enum
};

// Note on 'striped' vs 'chunked' layout for map tiles in memory:
// 'striped' can be most performant for iterating over every tile in the map in order
// 'chunked' (minecraft-like) can be most performant for operating on smaller parts of the map at a time (e.g. 3x3 cookie that moves over a local area)
// As the map size increases, striped layouts should fall farther behind chunked, in the areas where the chunked approach excels
typedef struct {
    int width;
    int height;
    // Left to right, then top to bottom
    htw_geo_MapTile *tiles;
} htw_TileMap;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t maxMagnitude;
    int32_t *values;
} htw_ValueMap;

// Allocates a map and enough space for all map elements
htw_TileMap *createTileMap(uint32_t width, uint32_t height);
htw_geo_MapTile *getMapTile(htw_TileMap *map, int x, int y);
void setMapTile(htw_TileMap *map, htw_geo_MapTile tile, int x, int y);
void printTileMap(htw_TileMap *map);

htw_ValueMap *htw_geo_createValueMap(u32 width, u32 height, s32 maxValue);
s32 htw_geo_getMapValue(htw_ValueMap *map, u32 x, u32 y);
void htw_geo_setMapValue(htw_ValueMap *map, s32 value, u32 x, u32 y);
void printValueMap(htw_ValueMap *map);

/* Utilities for neighbors and tile position */
void htw_geo_indexToCoords(u32 index, u32 width, u32 *x, u32 *y);

void htw_geo_getHexCellPositionSkewed(s32 x, s32 y, float *xPos, float *yPos);

void htw_getHexCellPositionStaggered(int x, int y, float *xPos, float *yPos);

/* Map generation */
void htw_geo_fillGradient(htw_ValueMap* map, int gradStart, int gradEnd);

void htw_geo_fillNoise(htw_ValueMap* map, u32 seed);

void htw_geo_fillSmoothNoise(htw_ValueMap* map, u32 seed, float scale);

void htw_geo_fillPerlin(htw_ValueMap* map, u32 seed, u32 octaves, float scale);

#endif
