#ifndef HTW_GEOMAP_H_INCLUDED
#define HTW_GEOMAP_H_INCLUDED

/* Notes on coordinate systems:
 * Maps may be used on their own, or may be part of a larger grid of 'chunks' used to limit the size of each individual map while creating very large worlds
 * This can lead to some confusion about what coordinate values mean in different contexts, as well as performance concerns when iterating over many cells
 *
 * Here some terms will be defined to help make their use consistent throughout this project:
 * index - unsigned integer
 * coordinate - signed integer
 * position - float
 * cell - An element contained in a map. Size and type is determined by the type of map (one of the predefined types, or void type with size set by application code)
 * map - Array of cells with a set width and height where length = width * height. Can be accessed by raw index or (x, y) coords (typically as an htw_geo_GridCoord struct)
 * chunk - Map used as part of a larger array of maps
 * cell index - raw index into map data for a single map
 * cell coordinate / cell x / cell y - map-local integer coordinates for a cell. Only valid in [0, map_dimension_max), otherwise access will be out of bounds (TODO: different access methods, or maybe a define option to change between bounds checking and wrapping for each direction?)
 * grid coordinate / grid x / grid y - generic integer coordinates, used for any absolute or relative 2d position. If used for global coordinates in chunk-based applications, will need conversion before using as cell coordinates
 * world position - global float position. Only supported in the form of conversion from hex grid coordinates to world space positions
 * cube coordinates - special coordinate system used by some hexmap algorithms, see htw_geomap_hexgrid.c for details
 *
 * Usage guidelines:
 * When iterating through whole rows of a map, access by cell index should be preferred
 * Chunked systems will ofter have to convert cell references from chunkIndex+cellIndex to world grid coordinates
 *
 */

#include <stdint.h>
#include "htw_core.h"

// useful constants; based on distance between 2 adjacent hex centers == 1
static const float htw_geo_HexOuterRadius = 0.57735026919; // distance from center to corner = sqrt(0.75) * (2.0 / 3.0);
static const float htw_geo_HexInnerRadius = 0.5; // distance from center to edge
static const float htw_geo_HexEdgeLength = htw_geo_HexInnerRadius;
static const float htw_geo_HexHalfEdgeLength = htw_geo_HexEdgeLength / 2;

typedef struct {
    s32 x;
    s32 y;
} htw_geo_GridCoord;

typedef struct {
    s32 q;
    s32 r;
    s32 s;
} htw_geo_CubeCoord;

static const htw_geo_CubeCoord htw_geo_cubeDirections[] = {
    (htw_geo_CubeCoord){0, 1, -1},
    (htw_geo_CubeCoord){1, 0, -1},
    (htw_geo_CubeCoord){1, -1, 0},
    (htw_geo_CubeCoord){0, -1, 1},
    (htw_geo_CubeCoord){-1, 0, 1},
    (htw_geo_CubeCoord){-1, 1, 0},
};

static const htw_geo_GridCoord htw_geo_hexGridDirections[] = {
    (htw_geo_GridCoord){0, 1},
    (htw_geo_GridCoord){1, 0},
    (htw_geo_GridCoord){1, -1},
    (htw_geo_GridCoord){0, -1},
    (htw_geo_GridCoord){-1, 0},
    (htw_geo_GridCoord){-1, 1},
};

/// For a 'unit' inner diameter = 1 hex. Starts at top middle, runs clockwise
static const float htw_geo_hexCornerPositions[6][2] = {
    {0.0, htw_geo_HexOuterRadius},
    {htw_geo_HexInnerRadius, htw_geo_HexHalfEdgeLength},
    {htw_geo_HexInnerRadius, -htw_geo_HexHalfEdgeLength},
    {0.0, -htw_geo_HexOuterRadius},
    {-htw_geo_HexInnerRadius, -htw_geo_HexHalfEdgeLength},
    {-htw_geo_HexInnerRadius, htw_geo_HexHalfEdgeLength},
};

/** Corresponds to the locations below on a square grid:
 * 5 0 -
 * 4 - 1
 * - 3 2
 *
 * Also corresponds to elements of htw_geo_cubeDirections and htw_geo_hexGridDirections
 */
typedef enum HexDirection {
    HEX_DIRECTION_NORTH_EAST = 0, // 0, 1
    HEX_DIRECTION_EAST, // 1, 0
    HEX_DIRECTION_SOUTH_EAST, // 1, -1
    HEX_DIRECTION_SOUTH_WEST, // 0, -1
    HEX_DIRECTION_WEST, // -1, 0
    HEX_DIRECTION_NORTH_WEST, // -1, 1
    HEX_DIRECTION_COUNT // Limit for iterating over values in this enum
} HexDirection;

/** Starts from the top and winds around clockwise
 *
 * Also corresponds to elements of htw_geo_hexCornerPositions
 */
typedef enum HexCorner {
    HEX_CORNER_NORTH = 0,
    HEX_CORNER_NORTH_EAST,
    HEX_CORNER_SOUTH_EAST,
    HEX_CORNER_SOUTH,
    HEX_CORNER_SOUTH_WEST,
    HEX_CORNER_NORTH_WEST,
    HEX_CORNER_COUNT // Limit for iterating over values in this enum
} HexCorner;

// Note on 'striped' vs 'chunked' layout for map tiles in memory:
// 'striped' can be most performant for iterating over every tile in the map in order
// 'chunked' (minecraft-like) can be most performant for operating on smaller parts of the map at a time (e.g. 3x3 cookie that moves over a local area)
// As the map size increases, striped layouts should fall farther behind chunked, in the areas where the chunked approach excels
typedef struct {
    void *cellData;
} htw_Chunk;

typedef struct {
    u32 chunkSize; // width and height of each chunk
    u32 chunkCountX, chunkCountY; // number of chunks along each axis
    u32 mapWidth, mapHeight; // total map dimensions
    u32 cellsPerChunk;
    size_t cellDataSize;
    htw_Chunk *chunks;
} htw_ChunkMap;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t maxMagnitude;
    int32_t *values;
} htw_ValueMap;

typedef struct htw_Link {
    struct htw_Link *next;
} htw_Link;

typedef struct {
    size_t maxItemCount;
    size_t hashSlotCount;
    u32 hashBitMask;
    htw_Link *links;
    htw_Link **hashSlots;
} htw_SpatialStorage;

// Allocates a map and enough space for all map elements
htw_ChunkMap *htw_geo_createChunkMap(u32 chunkSize, u32 chunkCountX, u32 chunkCountY, size_t cellDataSize);
// NOTE: no out of bounds access possible through these methods, coordinates always wrap based on map size
void *htw_geo_getCell(const htw_ChunkMap *chunkMap, htw_geo_GridCoord cellCoord);
u32 htw_geo_getChunkIndexByChunkCoordinates(const htw_ChunkMap *chunkMap, htw_geo_GridCoord chunkCoord);
u32 htw_geo_getChunkIndexByGridCoordinates(const htw_ChunkMap *chunkMap, htw_geo_GridCoord gridCoord);
u32 htw_geo_getChunkIndexAtOffset(const htw_ChunkMap *chunkMap, u32 startingChunk, htw_geo_GridCoord chunkOffset);
htw_geo_GridCoord htw_geo_chunkIndexToChunkCoordinates(const htw_ChunkMap *chunkMap, u32 chunkIndex);
void htw_geo_gridCoordinateToChunkAndCellIndex(const htw_ChunkMap *chunkMap, htw_geo_GridCoord gridCoord, u32 *chunkIndex, u32 *cellIndex);
htw_geo_GridCoord htw_geo_chunkAndCellToGridCoordinates(const htw_ChunkMap *chunkMap, u32 chunkIndex, u32 cellIndex);
void htw_geo_getChunkRootPosition(const htw_ChunkMap *chunkMap, u32 chunkIndex, float *worldX, float *worldY);
htw_geo_GridCoord htw_geo_wrapGridCoordOnChunkMap(const htw_ChunkMap *chunkMap, htw_geo_GridCoord coord);
htw_geo_GridCoord htw_geo_addGridCoordsWrapped(const htw_ChunkMap *chunkMap, htw_geo_GridCoord a, htw_geo_GridCoord b);
/// Magnitude of any dimension of a vector in wrapped space will never be greater than 1/2 the extent of the same dimension of the space
htw_geo_GridCoord htw_geo_wrapVectorOnChunkMap(const htw_ChunkMap *chunkMap, htw_geo_GridCoord vec);
u32 htw_geo_getChunkMapHexDistance(const htw_ChunkMap *chunkMap, htw_geo_GridCoord a, htw_geo_GridCoord b);
float htw_geo_hexCartesianDistance(const htw_ChunkMap *chunkMap, htw_geo_GridCoord a, htw_geo_GridCoord b);

htw_ValueMap *htw_geo_createValueMap(u32 width, u32 height, s32 maxValue);
s32 htw_geo_getMapValueByIndex(htw_ValueMap *map, u32 cellIndex);
s32 htw_geo_getMapValue(htw_ValueMap *map, htw_geo_GridCoord cellCoord);
void htw_geo_setMapValueByIndex(htw_ValueMap *map, u32 cellIndex, s32 value);
void htw_geo_setMapValue(htw_ValueMap *map, htw_geo_GridCoord cellCoord, s32 value);
void printValueMap(htw_ValueMap *map);

/* Grid coord math, neighbors, and index conversion */
htw_geo_GridCoord htw_geo_indexToGridCoord(u32 cellIndex, u32 mapWidth);
u32 htw_geo_cellCoordToIndex(htw_geo_GridCoord cellCoord, u32 mapWidth);
u32 htw_geo_isEqualGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b);
htw_geo_GridCoord htw_geo_addGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b);
/// subtract b from a
htw_geo_GridCoord htw_geo_subGridCoords(htw_geo_GridCoord a, htw_geo_GridCoord b);
HexDirection htw_geo_hexDirectionLeft(HexDirection dir);
HexDirection htw_geo_hexDirectionRight(HexDirection dir);
HexDirection htw_geo_hexDirectionOpposite(HexDirection dir);
/// Return HexDirection 'slice' that the endpoint of vec lies in
HexDirection htw_geo_vectorHexDirection(htw_geo_GridCoord vec);
/// Return the hexDirection of b from a. Exact only for coaxial grid coords, otherwise will be the same for every coord in a 'slice' between 2 axies. For best results, use coords with distance(a, b) == 1. If a == b, returns -1
HexDirection htw_geo_relativeHexDirection(htw_geo_GridCoord a, htw_geo_GridCoord b);

/* Cube coord math and gridcoord conversion */
u32 htw_geo_isEqualCubeCoords(htw_geo_CubeCoord a, htw_geo_CubeCoord b);
htw_geo_CubeCoord htw_geo_addCubeCoords(htw_geo_CubeCoord a, htw_geo_CubeCoord b);
htw_geo_CubeCoord htw_geo_gridToCubeCoord(htw_geo_GridCoord gridCoord);
htw_geo_GridCoord htw_geo_cubeToGridCoord(htw_geo_CubeCoord cubeCoord);

// Macro to get position in a hex direction
#define POSITION_IN_DIRECTION(pos, dir) htw_geo_addGridCoords(pos, htw_geo_hexGridDirections[dir])

/* Grid coord and cartesian position conversion */
float htw_geo_cartesianToHexPositionX(float x, float y);
float htw_geo_cartesianToHexPositionY(float y);
float htw_geo_hexToCartesianPositionX(float x, float y);
float htw_geo_hexToCartesianPositionY(float y);
float htw_geo_getHexPositionX(s32 gridX, s32 gridY);
float htw_geo_getHexPositionY(s32 gridY);
void htw_geo_getHexCellPositionSkewed(htw_geo_GridCoord gridCoord, float *xPos, float *yPos);
void htw_geo_cartesianToHexFractional(float x, float y, float *q, float *r);
htw_geo_GridCoord htw_geo_hexFractionalToHexCoord(float q, float r);
htw_geo_GridCoord htw_geo_cartesianToHexCoord(float x, float y);
u32 htw_geo_cartesianToHexChunkIndex(htw_ChunkMap *chunkMap, float x, float y);

/* Hex grid geometry */
u32 htw_geo_hexGridDistance(htw_geo_GridCoord a, htw_geo_GridCoord b);
u32 htw_geo_hexCubeDistance(htw_geo_CubeCoord a, htw_geo_CubeCoord b);
u32 htw_geo_hexGridMagnitude(htw_geo_GridCoord vec);
u32 htw_geo_hexCubeMagnitude(htw_geo_CubeCoord vec);
u32 htw_geo_getHexArea(u32 edgeLength);
void htw_geo_getNextHexSpiralCoord(htw_geo_CubeCoord *iterCoord);

/* Spatial Hashmaps */
htw_SpatialStorage *htw_geo_createSpatialStorage(size_t maxItemCount);
void htw_geo_spatialInsert(htw_SpatialStorage *ss, htw_geo_GridCoord coord, size_t itemIndex);
void htw_geo_spatialRemove(htw_SpatialStorage *ss, htw_geo_GridCoord coord, size_t itemIndex);
void htw_geo_spatialMove(htw_SpatialStorage *ss, htw_geo_GridCoord oldCoord, htw_geo_GridCoord newCoord, size_t itemIndex);
size_t htw_geo_spatialItemCountAt(htw_SpatialStorage *ss, htw_geo_GridCoord coord);
size_t htw_geo_spatialFirstIndex(htw_SpatialStorage *ss, htw_geo_GridCoord coord);

/* Map generation */
/* For filling entire valueMaps: */
void htw_geo_fillUniform(htw_ValueMap *map, s32 uniformValue);
void htw_geo_fillChecker(htw_ValueMap *map, s32 value1, s32 value2, u32 gridOrder);
void htw_geo_fillGradient(htw_ValueMap* map, int gradStart, int gradEnd);
void htw_geo_fillCircularGradient(htw_ValueMap* map, htw_geo_GridCoord center, s32 gradStart, s32 gradEnd, float radius);
void htw_geo_fillNoise(htw_ValueMap* map, u32 seed);
void htw_geo_fillSmoothNoise(htw_ValueMap* map, u32 seed, float scale);
void htw_geo_fillPerlin(htw_ValueMap* map, u32 seed, u32 octaves, s32 posX, s32 posY, float scale, float repeatX, float repeatY);
void htw_geo_fillSimplex(htw_ValueMap* map, u32 seed, u32 octaves, s32 posX, s32 posY, u32 repeatInterval, u32 samplesPerRepeat);

/* For filling entire chunkMaps: */
// TODO?

/* For getting single cell values: */
s32 htw_geo_circularGradientByGridCoord(const htw_ChunkMap *chunkMap, htw_geo_GridCoord cellCoord, htw_geo_GridCoord center, s32 gradStart, s32 gradEnd, float radius);
/**
 * @brief Simplex noise value for single cell on a hexagonal gridmap
 *
 * @param chunkMap Reference chunkmap, map size is used to ensure smooth wrapping
 * @param cellCoord
 * @param seed
 * @param octaves Number of cycles to use when computing noise. Higher values will create more detailed noise, but take longer to evaluate
 * @param samplesPerRepeat Effects size of the first octave. Higher values will create a noiser result
 * @return float
 */
float htw_geo_simplex(htw_ChunkMap *chunkMap, htw_geo_GridCoord cellCoord, u32 seed, u32 octaves, u32 samplesPerRepeat);

#endif
