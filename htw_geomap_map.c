#include <stdio.h>
#include "htw_geomap.h"


Map *createMap(int width, int height) {
    Map *newMap = (Map*)malloc(sizeof(Map));
    newMap->width = width;
    newMap->height = height;
    // create top level list
    newMap->cells = *createList(sizeof(List), width);
    // create nested lists
    for (int i = 0; i < width; i++) {
        *(List*)getItem(&newMap->cells, i) = *createList(sizeof(List), height);
    }
    return newMap;
}

void printCell(void *val) {
    printf("%i", *(int*)val);
}

void printMap(Map *map) {
    for (int x = 0; x < map->width; x++) {
        List *column = (List*)getItem(&map->cells, x);
        printList(column, &printCell);
    }
    printf("\n");
}
