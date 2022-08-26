#include <stdio.h>
#include <math.h>
#include "htw_geomap.h"


TileMap *createTileMap (int width, int height) {
    int fullSize = sizeof(TileMap) + (sizeof( MapTile ) * width * height);
    TileMap *newMap = ( TileMap*)malloc(fullSize);
    newMap->width = width;
    newMap->height = height;
    newMap->tiles = (MapTile*)(newMap + 1);
    return newMap;
}

MapTile *getMapTile ( TileMap *map, int x, int y) {
    return &map->tiles[(y * map->width) + x];
}

void setMapTile ( TileMap *map, MapTile tile, int x, int y) {
    *getMapTile (map, x, y) = tile;
}

void printCell(void *val) {
    printf("%i", *(int*)val);
}

void printTileMap ( TileMap *map) {
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int value = map->tiles[(y * map->width) + x].id;
            printf("%i", value);
        }
        printf("\n");
    }
}

void htw_getHexCellPositionSkewed(int x, int y, float *xPos, float *yPos) {
    *yPos = sqrt(0.75) * y;
    *xPos = x + ((float)y * 0.5);
}

void htw_getHexCellPositionStaggered(int x, int y, float *xPos, float *yPos) {
    *yPos = sqrt(0.75) * y;
    if (y % 2 == 1) *xPos = (double)x;
    else *xPos = (double)x + 0.5;
}
