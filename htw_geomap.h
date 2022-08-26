#ifndef HTW_GEOMAP_H_INCLUDED
#define HTW_GEOMAP_H_INCLUDED

#include "htw_core.h"

#define TILE_NAME_MAX_LENGTH 256

typedef struct {
    int id;
    void *content;
} MapTile;

void *loadTileDefinitions (char *path);
MapTile *newTile (int id);
char *getTileName (int id);

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
    MapTile *tiles;
} TileMap;

typedef struct {
    int width;
    int height;
    int maxValue;
    int *values;
} ValueMap;

// Allocates a map and enough space for all map elements
TileMap *createTileMap (int width, int height);
MapTile *getMapTile ( TileMap *map, int x, int y);
void setMapTile ( TileMap *map, MapTile tile, int x, int y);
void printTileMap ( TileMap *map);

ValueMap *createValueMap(int width, int height, int maxValue);
int getMapValue (ValueMap *map, int x, int y);
void setMapValue (ValueMap *map, int value, int x, int y);
void printValueMap (ValueMap *map);

/* Utilities for neighbors and tile position */
void htw_getHexCellPositionSkewed(int x, int y, float *xPos, float *yPos);

void htw_getHexCellPositionStaggered(int x, int y, float *xPos, float *yPos);

/* Map generation */
void fillGradient ( ValueMap* map, int gradStart, int gradEnd );


#endif
