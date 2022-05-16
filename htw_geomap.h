#ifndef HTW_GEOMAP_H_INCLUDED
#define HTW_GEOMAP_H_INCLUDED

#include "htw_core.h"

typedef struct {
    int id;
} MapCell;

// Note on 'striped' vs 'chunked' layout for map cells in memory:
// 'striped' can be most performant for iterating over every cell in the map in order
// 'chunked' (minecraft-like) can be most performant for operating on smaller parts of the map at a time (e.g. 3x3 cookie that moves over a local area)
// As the map size increases, striped layouts will fall farther behind chunked, in the areas where the chunked approach excels
typedef struct {
    int width;
    int height;
    // List of list* of int (something else later)
    MapCell *cells;
} Map;

Map *createMap(int width, int height);
MapCell *getCell(Map *map, int x, int y);

void printCell(void *val);
void printMap(Map *map);

#endif
