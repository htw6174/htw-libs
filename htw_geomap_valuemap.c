#include <stdio.h>
#include "htw_geomap.h"

htw_ValueMap *htw_geo_createValueMap(u32 width, u32 height, s32 maxValue) {
    int fullSize = sizeof(htw_ValueMap) + (sizeof(int) * width * height);
    htw_ValueMap *newMap = (htw_ValueMap*)malloc(fullSize);
    newMap->width = width;
    newMap->height = height;
    newMap->maxMagnitude = maxValue;
    newMap->values = (int*)(newMap + 1);
    return newMap;
}

s32 htw_geo_getMapValue(htw_ValueMap *map, u32 x, u32 y) {
    return map->values[(y * map->width) + x];
}

void htw_geo_setMapValue(htw_ValueMap *map, s32 value, u32 x, u32 y) {
    if (x >= map->width || y >= map->height) {
        fprintf(stderr, "coordinates outside map range: %u, %u", x, y);
        return;
    }
    map->values[(y * map->width) + x] = value;
}

void printValueMap ( htw_ValueMap *map) {
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int value = map->values[(y * map->width) + x];
            printf("%i", value);
        }
        printf("\n");
    }
}
