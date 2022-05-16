#include <stdio.h>
#include "htw_geomap.h"


Map *createMap(int width, int height) {
    Map *newMap = (Map*)malloc(sizeof(Map));
    newMap->width = width;
    newMap->height = height;
    newMap->cells = malloc(sizeof(MapCell) * width * height);
    return newMap;
}

MapCell *getCell(Map *map, int x, int y) {
    return &map->cells[(y * map->width) + x];
}

void printCell(void *val) {
    printf("%i", *(int*)val);
}

void printMap(Map *map) {
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int value = map->cells[(y * map->width) + x].id;
            printf("%i", value);
        }
        printf("\n");
    }
}
