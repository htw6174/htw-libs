#include <math.h>
#include "htw_core.h"
#include "htw_random.h"
#include "htw_geomap.h"

void htw_geo_fillGradient(htw_ValueMap *map, int gradStart, int gradEnd ) {
    gradStart = min_int( gradStart, map->maxMagnitude);
    gradEnd = min_int( gradEnd, map->maxMagnitude);
    for (int y = 0; y < map->height; y++) {
        int gradCurrent = lerp_int( gradStart, gradEnd, (double)y / map->height );
        for (int x = 0; x < map->width; x++) {
            htw_geo_setMapValue(map, gradCurrent, x, y);
        }
    }
}

void htw_geo_fillNoise(htw_ValueMap* map, u32 seed) {
    for (u32 y = 0; y < map->height; y++) {
        for (u32 x = 0; x < map->width; x++) {
            u32 val = xxh_hash2d(seed, x, y);
            val = val % map->maxMagnitude;
            htw_geo_setMapValue(map, val, x, y);
        }
    }
}

void htw_geo_fillSmoothNoise(htw_ValueMap* map, u32 seed, float scale) {
    for (u32 y = 0; y < map->height; y++) {
        for (u32 x = 0; x < map->width; x++) {
            float scaledX, scaledY;
            htw_geo_getHexCellPositionSkewed(x, y, &scaledX, &scaledY);
            scaledX *= scale;
            scaledY *= scale;
            float val = htw_value2d(seed, scaledX, scaledY);
            val = floorf(val * map->maxMagnitude);
            htw_geo_setMapValue(map, val, x, y);
        }
    }
}

void htw_geo_fillPerlin(htw_ValueMap* map, u32 seed, u32 octaves, float scale) {
    for (u32 y = 0; y < map->height; y++) {
        for (u32 x = 0; x < map->width; x++) {
            float scaledX, scaledY;
            htw_geo_getHexCellPositionSkewed(x, y, &scaledX, &scaledY);
            scaledX *= scale;
            scaledY *= scale;
            float val = htw_perlin2d(seed, scaledX, scaledY, octaves);
            val = floorf(val * map->maxMagnitude);
            htw_geo_setMapValue(map, val, x, y);
        }
    }
}
