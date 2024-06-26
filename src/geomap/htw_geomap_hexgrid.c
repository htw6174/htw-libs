
#include <math.h>
#include "htw_core.h"
#include "htw_geomap.h"

// similar to Red Blob Game's example (https://www.redblobgames.com/grids/hexagons/), cube (3-axis) hexagon coordinates use (q, r, s) to represent cell position. Skewed (RBG calls this 'axial') coordinates use (x, y) for cell position, because they only differ from cartesian grid coordinates in how they are converted to world space positions, which can be distinguished by the use of int (for grid position) or float (for world position) types
// In this library, horizontal or 'pointy topped' hexagons arranged with +x skew are always used.
// There are differences from the redblob examples because of the +x skew: to translate their coordinate system to this one it must be flipped across the r=0 axis
// For most algorithms these difference don't matter, but it does affect converting from hex/cube coords to cartesian and back
// Do this flip by swapping q and s, and negating the coord
// Note that the conversion scale from hex to cartesian is also different from redblob's page
// Here, the center-to-center distance between hexagons translates to 1 unit in cartesian space, i.e. the width/inner diameter of hexes is 1

// Some alogrithms are easier to implement with cube coordinates
// When converting between skewed hex and cartesian coordinates: +x is due right, -x is due left, +y is top-right, -y is bottom-left
// When converting between skewed hex and cube hex coordinates: +/-x = +/-q, +/-y = +/-r, s = -q - r
// Cube space to world space directions are more complicated, because all movements involve changing 2 dimensions:
// +q/-s is right, -q/+s is left, +r/-s is top-right, -r/+s is bottom-left, +q/-r is bottom-right, -q/+r is top-left
// q=0 axis runs from bottom-left to top-right, r=0 axis runs from left to right, and s=0 axis runs from bottom-right to top-left
// +q is ExSE, +r is due North, +s is WxSW
// cube coordinates must always adhere to the constraint q + r + s = 0

static inline s32 cube3rdAxis(s32 q, s32 r) {
    return -q - r;
}

htw_geo_GridCoord htw_geo_indexToGridCoord(u32 cellIndex, u32 mapWidth) {
    return (htw_geo_GridCoord){cellIndex % mapWidth, cellIndex / mapWidth};
}

u32 htw_geo_cellCoordToIndex(htw_geo_GridCoord cellCoord, u32 mapWidth) {
    return cellCoord.x + (cellCoord.y * mapWidth);
}

u32 htw_geo_isEqualGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return a.x == b.x && a.y == b.y;
}

htw_geo_GridCoord htw_geo_addGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return (htw_geo_GridCoord){a.x + b.x, a.y + b.y};
}

htw_geo_GridCoord htw_geo_subGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return (htw_geo_GridCoord){a.x - b.x, a.y - b.y};
}

HexDirection htw_geo_hexDirectionLeft(HexDirection dir) {
    //return MOD(dir - 1, HEX_DIRECTION_COUNT);
    return (dir + (HEX_DIRECTION_COUNT - 1)) % HEX_DIRECTION_COUNT;
}

HexDirection htw_geo_hexDirectionRight(HexDirection dir) {
    return (dir + 1) % HEX_DIRECTION_COUNT;
}

HexDirection htw_geo_hexDirectionOpposite(HexDirection dir) {
    return (dir + 3) % HEX_DIRECTION_COUNT;
}

static const HexDirection relLUT[3][3] = {
    {HEX_DIRECTION_SOUTH_WEST, HEX_DIRECTION_SOUTH_WEST, HEX_DIRECTION_SOUTH_EAST},
    {HEX_DIRECTION_WEST      , -1                      , HEX_DIRECTION_EAST      },
    {HEX_DIRECTION_NORTH_WEST, HEX_DIRECTION_NORTH_EAST, HEX_DIRECTION_NORTH_EAST},
};

HexDirection htw_geo_vectorHexDirection(htw_geo_GridCoord vec) {
    s32 dirX = SIGN(vec.x);
    s32 dirY = SIGN(vec.y);
    return relLUT[dirY+1][dirX+1];
}

HexDirection htw_geo_relativeHexDirection(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return htw_geo_vectorHexDirection(htw_geo_subGridCoords(b, a));
}

htw_geo_CubeCoord htw_geo_addCubeCoords(htw_geo_CubeCoord a, htw_geo_CubeCoord b) {
    return (htw_geo_CubeCoord){a.q + b.q, a.r + b.r, a.s + b.s};
}

htw_geo_CubeCoord htw_geo_gridToCubeCoord(htw_geo_GridCoord gridCoord) {
    return (htw_geo_CubeCoord){gridCoord.x, gridCoord.y, cube3rdAxis(gridCoord.x, gridCoord.y)};
}

u32 htw_geo_isEqualCubeCoords(htw_geo_CubeCoord a, htw_geo_CubeCoord b) {
    return a.q == b.q && a.r == b.r && a.s == b.s;
}

htw_geo_GridCoord htw_geo_cubeToGridCoord(htw_geo_CubeCoord cubeCoord) {
    return (htw_geo_GridCoord){cubeCoord.q, cubeCoord.r};
}

// sqrt(0.75) == sqrt(3)/2
float htw_geo_cartesianToHexPositionX(float x, float y) {
    return x - (y * (0.5 / sqrt(0.75)));
}

float htw_geo_cartesianToHexPositionY(float y) {
    return y * (1.0 / sqrt(0.75));
}

float htw_geo_hexToCartesianPositionX(float x, float y) {
    return x + (y * 0.5);
}

float htw_geo_hexToCartesianPositionY(float y) {
    return y * sqrt(0.75);
}

float htw_geo_getHexPositionX(s32 gridX, s32 gridY) {
    return gridX + ((float)gridY * 0.5);
}

float htw_geo_getHexPositionY(s32 gridY) {
    return sqrt(0.75) * gridY;
}

void htw_geo_getHexCellPositionSkewed(htw_geo_GridCoord gridCoord, float *xPos, float *yPos) {
    *yPos = sqrt(0.75) * gridCoord.y;
    *xPos = gridCoord.x + ((float)gridCoord.y * 0.5);
}

void htw_geo_cartesianToHexFractional(float x, float y, float *q, float *r) {
    *q = x - (y * (0.5 / sqrt(0.75)));
    *r = y * (1.0 / sqrt(0.75));
}

htw_geo_GridCoord htw_geo_hexFractionalToHexCoord(float q, float r) {
    // based on https://www.shadertoy.com/view/dtySDy by FordPerfect
    // 3rd fractional cube coord
    float s = -q - r;

    float qr = floorf(q - r); // 2 * distance along s=0 axis; REDBLOB: + to top-right       MINE: + to bottom-right
    float rs = floorf(r - s); // 2 * distance along q=0 axis; REDBLOB: + to bottom-right    MINE: + to top-right
    float sq = floorf(s - q); // 2 * distance along r=0 axis; REDBLOB: + to left            MINE: + to left
    float qi = roundf((qr - sq)/3.0);
    float ri = roundf((rs - qr)/3.0);
    return (htw_geo_GridCoord){qi, ri};
}

htw_geo_GridCoord htw_geo_cartesianToHexCoord(float xCart, float yCart) {
    float q, r;
    htw_geo_cartesianToHexFractional(xCart, yCart, &q, &r);
    return htw_geo_hexFractionalToHexCoord(q, r);
}

u32 htw_geo_cartesianToHexChunkIndex(htw_ChunkMap *chunkMap, float x, float y) {
    // reverse hex grid coordinate skewing
    float deskewedY = (1.0 / sqrt(0.75)) * y;
    float deskewedX = x - (deskewedY * 0.5);
    // convert to chunk grid coordinates
    htw_geo_GridCoord chunkCoord = {
        .x = floorf(deskewedX / chunkMap->chunkSize),
        .y = floorf(deskewedY / chunkMap->chunkSize)
    };
    return htw_geo_getChunkIndexByChunkCoordinates(chunkMap, chunkCoord);
}

u32 htw_geo_hexGridDistance(htw_geo_GridCoord a, htw_geo_GridCoord b) {
    return (abs(a.x - b.x) + abs(a.x + a.y - b.x - b.y) + abs(a.y - b.y)) / 2;
}

u32 htw_geo_hexCubeDistance(htw_geo_CubeCoord a, htw_geo_CubeCoord b) {
    return (abs(a.q - b.q) + abs(a.r - b.r) + abs(a.s - b.s)) / 2;
}

u32 htw_geo_hexGridMagnitude(htw_geo_GridCoord vec) {
    return (abs(vec.x) + abs(-vec.x - vec.y) + abs(vec.y)) / 2;
}

u32 htw_geo_hexCubeMagnitude(htw_geo_CubeCoord vec) {
    return (abs(vec.q) + abs(vec.r) + abs(vec.s)) / 2;
}

// perimeter given in cells. edgeLength 0 is undefined.
u32 htw_geo_getHexPerimeter(u32 edgeLength) {
    return MAX(6 * (edgeLength - 1), 1);
}

// quadratic form of this formula: find area of square enclosing the hexagonal area (edge length = 2n - 1), and subtract the area of the corners that aren't in the hexagon
// 1 -> 1, 2 -> 7, etc.
// NOTE: 0 -> 1
u32 htw_geo_getHexArea(u32 edgeLength) {
    return (3 * (edgeLength * edgeLength)) - (3 * edgeLength) + 1;
}

// Sets the value of iterCoord to be the next coordinate in an outward hexagon spiral starting from (0, 0, 0), moving outward at (q=0 && r >=s) towards +r and continuing clockwise
void htw_geo_getNextHexSpiralCoord(htw_geo_CubeCoord *iterCoord) {
    if (iterCoord->q == 0 && iterCoord->r >= iterCoord->s) { // true if coord is on the +x axis in cartesian space
        // move left to next ring
        *iterCoord = htw_geo_addCubeCoords(*iterCoord, htw_geo_cubeDirections[HEX_DIRECTION_EAST]);
    } else {
        // determine which 'wedge' (area between 2 direction lines) the coord is in
        // NOTE: is there any way to do this with fewer checks?
        s32 wedge = -1;
        if (iterCoord->q >= 0 && iterCoord->r > 0) wedge = 0;
        if (iterCoord->r <= 0 && iterCoord->s < 0) wedge = 1;
        if (iterCoord->s >= 0 && iterCoord->q > 0) wedge = 2;
        if (iterCoord->q <= 0 && iterCoord->r < 0) wedge = 3;
        if (iterCoord->r >= 0 && iterCoord->s > 0) wedge = 4;
        if (iterCoord->s <= 0 && iterCoord->q < 0) wedge = 5;

        wedge = (wedge + 2) % HEX_DIRECTION_COUNT; // translate to required movement direction
        *iterCoord = htw_geo_addCubeCoords(*iterCoord, htw_geo_cubeDirections[wedge]);
    }
}

void htw_geo_getHexRingPosition(u32 edgeLength, u32 n, s32 *x, s32 *y) {
    // TODO
}
