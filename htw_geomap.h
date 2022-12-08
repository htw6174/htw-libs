#ifndef HTW_GEOMAP_H_INCLUDED
#define HTW_GEOMAP_H_INCLUDED

/* Notes on coordinate systems:
 * Maps may be used on their own, or may be part of a larger grid of 'chunks' used to limit the size of each individual map while creating very large worlds
 * This can lead to some confusion about what coordinate values mean in different contexts, as well as performance concerns when iterating over many cells
 *
 * Here some terms will be defined to help make their use consistent throughout this project:
 * index - unsigned integer
 * coordinate - signed integer
 * position - float
 * cell - An element contained in a map. Size and type is determined by the type of map (one of the predefined types, or void type with size set by application code)
 * map - Array of cells with a set width and height where length = width * height. Can be accessed by raw index or (x, y) coords (typically as an htw_geo_GridCoord struct)
 * chunk - Map used as part of a larger array of maps. Not directly supported by htw_geomap.
 * cell index - raw index into map data for a single map
 * cell coordinate / cell x / cell y - map-local integer coordinates for a cell. Only valid in [0, map_dimension_max), otherwise access will be out of bounds (TODO: different access methods, or maybe a define option to change between bounds checking and wrapping for each direction?)
 * grid coordinate / grid x / grid y - generic integer coordinates, used for any absolute or relative 2d position. If used for global coordinates in chunk-based applications, will need conversion before using as cell coordinates
 * world position - global float position. Only supported in the form of conversion from hex grid coordinates to world space positions
 * cube coordinates - special coordinate system used by some hexmap algorithms, see htw_geomap_hexgrid.c for details
 *
 * Usage guidelines:
 * When iterating through whole rows of a map, access by cell index should be preferred
 * Chunked systems will ofter have to convert cell references from chunkIndex+cellIndex to world grid coordinates
 *
 */

#include <stdint.h>
#include "htw_core.h"

#define HTW_GEO_TILE_NAME_MAX_LENGTH 256

typedef struct {
    int id;
    void *content;
} htw_geo_MapTile;

void *htw_geo_loadTileDefinitions (char *path);
htw_geo_MapTile *htw_createTile (int id); // FIXME: no reason to allocate one map tile at a time. Remove when reworking tilemap system
char *htw_geo_getTileName (int id);

typedef struct {
    s32 x;
    s32 y;
} htw_geo_GridCoord;

typedef struct {
    s32 q;
    s32 r;
    s32 s;
} htw_geo_CubeCoord;

static const htw_geo_CubeCoord htw_geo_cubeDirections[] = {
    (htw_geo_CubeCoord){0, 1, -1},
    (htw_geo_CubeCoord){1, 0, -1},
    (htw_geo_CubeCoord){1, -1, 0},
    (htw_geo_CubeCoord){0, -1, 1},
    (htw_geo_CubeCoord){-1, 0, 1},
    (htw_geo_CubeCoord){-1, 1, 0},
};

/** Corresponds to the locations below on a square grid:
 * 5 0 -
 * 4 - 1
 * - 3 2
 *
 * Also corresponds to elements of htw_geo_cubeDirections
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
// TODO: remove this or replace it with a generic void* version of value maps
htw_TileMap *createTileMap(uint32_t width, uint32_t height);
htw_geo_MapTile *getMapTile(htw_TileMap *map, int x, int y);
void setMapTile(htw_TileMap *map, htw_geo_MapTile tile, int x, int y);
void printTileMap(htw_TileMap *map);

htw_ValueMap *htw_geo_createValueMap(u32 width, u32 height, s32 maxValue);
s32 htw_geo_getMapValueByIndex(htw_ValueMap *map, u32 cellIndex);
s32 htw_geo_getMapValue(htw_ValueMap *map, htw_geo_GridCoord cellCoord);
void htw_geo_setMapValueByIndex(htw_ValueMap *map, u32 cellIndex, s32 value);
void htw_geo_setMapValue(htw_ValueMap *map, htw_geo_GridCoord cellCoord, s32 value);
void printValueMap(htw_ValueMap *map);

/* Utilities for neighbors and tile position */
// TODO/TEST: any perf difference from inlining these?
static htw_geo_GridCoord htw_geo_indexToGridCoord(u32 cellIndex, u32 mapWidth);
static u32 htw_geo_cellCoordToIndex(htw_geo_GridCoord cellCoord, u32 mapWidth);
static u32 htw_geo_isEqualGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b);
static htw_geo_GridCoord htw_geo_addGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b);
// hexgrid.c
u32 htw_geo_isEqualCubeCoords(htw_geo_CubeCoord a, htw_geo_CubeCoord b);
htw_geo_CubeCoord htw_geo_addCubeCoords(htw_geo_CubeCoord a, htw_geo_CubeCoord b);
htw_geo_CubeCoord htw_geo_gridToCubeCoord(htw_geo_GridCoord gridCoord);
htw_geo_GridCoord htw_geo_cubeToGridCoord(htw_geo_CubeCoord cubeCoord);

float htw_geo_cartesianToHexPositionX(float x, float y);
float htw_geo_cartesianToHexPositionY(float y);
float htw_geo_hexToCartesianPositionX(float x, float y);
float htw_geo_hexToCartesianPositionY(float y);
float htw_geo_getHexPositionX(s32 gridX, s32 gridY);
float htw_geo_getHexPositionY(s32 gridY);
void htw_geo_getHexCellPositionSkewed(htw_geo_GridCoord gridCoord, float *xPos, float *yPos);

u32 htw_geo_getHexArea(u32 edgeLength);

/* Map generation */
void htw_geo_fillUniform(htw_ValueMap *map, s32 uniformValue);
void htw_geo_fillChecker(htw_ValueMap *map, s32 value1, s32 value2, u32 gridOrder);
void htw_geo_fillGradient(htw_ValueMap* map, int gradStart, int gradEnd);
void htw_geo_fillNoise(htw_ValueMap* map, u32 seed);
void htw_geo_fillSmoothNoise(htw_ValueMap* map, u32 seed, float scale);
void htw_geo_fillPerlin(htw_ValueMap* map, u32 seed, u32 octaves, s32 posX, s32 posY, float scale, float repeatX, float repeatY);
void htw_geo_fillSimplex(htw_ValueMap* map, u32 seed, u32 octaves, s32 posX, s32 posY, u32 repeatInterval, u32 samplesPerRepeat);



static htw_geo_GridCoord htw_geo_indexToGridCoord(u32 cellIndex, u32 mapWidth) {
    return (htw_geo_GridCoord){cellIndex % mapWidth, cellIndex / mapWidth};
}

static u32 htw_geo_cellCoordToIndex(htw_geo_GridCoord cellCoord, u32 mapWidth) {
    return cellCoord.x + (cellCoord.y * mapWidth);
}

static u32 htw_geo_isEqualGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return a.x == b.x && a.y == b.y;
}

void htw_geo_getNextHexSpiralCoord(htw_geo_CubeCoord *iterCoord);

static htw_geo_GridCoord htw_geo_addGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return (htw_geo_GridCoord){a.x + b.x, a.y + b.y};
}

#endif
