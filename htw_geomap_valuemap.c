#include <stdio.h>
#include "htw_geomap.h"

ValueMap *createValueMap(int width, int height, int maxValue) {
    int fullSize = sizeof(ValueMap) + (sizeof(int) * width * height);
    ValueMap *newMap = (ValueMap*)malloc(fullSize);
    newMap->width = width;
    newMap->height = height;
    newMap->maxValue = maxValue;
    newMap->values = (int*)(newMap + 1);
    return newMap;
}

int getMapValue (ValueMap *map, int x, int y) {
    return map->values[(y * map->width) + x];
}

void setMapValue (ValueMap *map, int value, int x, int y) {
    if (x >= map->width || y >= map->height) {
        fprintf(stderr, "coordinates outside map range: %i, %i", x, y);
        return;
    }
    map->values[(y * map->width) + x] = value;
}

void printValueMap ( ValueMap *map) {
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int value = map->values[(y * map->width) + x];
            printf("%i", value);
        }
        printf("\n");
    }
}
