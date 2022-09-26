#include <math.h>
#include "htw_core.h"
#include "htw_geomap.h"

void fillGradient(htw_ValueMap *map, int gradStart, int gradEnd ) {
    gradStart = min_int( gradStart, map->maxValue);
    gradEnd = min_int( gradEnd, map->maxValue);
    for (int y = 0; y < map->height; y++) {
        int gradCurrent = lerp_int( gradStart, gradEnd, (double)y / map->height );
        for (int x = 0; x < map->width; x++) {
            setMapValue(map, gradCurrent, x, y);
        }
    }
}
