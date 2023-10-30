#ifndef HTW_RANDOM_H_INCLUDED
#define HTW_RANDOM_H_INCLUDED

/**
 * @file htw_random.h
 * @brief These random number generation methods are designed for convenience and even distribution, NOT security
 */

#include "htw_core.h"

/**
 * @defgroup basics Basics
 * @{
 */

/// Random int in [0, max] (inclusive)
int htw_randInt(int max);

/// Random int in [0, size) (up to but not including size)
int htw_randIndex(int size);

/// Random int in [min, max] (inclusive)
int htw_randIntRange(int min, int max);

/// Random float in [0, 1]
float htw_randValue();

/// Random float in [min, max]
float htw_randRange(float min, float max);

/// Returns 0 or 1
int htw_coinFlip();

/// Returns 1 with probabality p (0 to 1); otherwise returns 0
int htw_weightedFlip(float p);

/** @}*/

/**
 * @defgroup advanced More complex distributions
 * @{
 */

/**
 * @brief Roll The Dice; get total and/or individual results for [count] dice rolls. Result for each roll will be in the range [1, sides]
 *
 * @param count number of dice to roll
 * @param sides number of sides on each die
 * @param results NULL or a pointer to an array of at least [count] elements, to record the result of each roll
 * @return sum of all rolls
 */
int htw_rtd(int count, int sides, int *results);

float htw_randNormal(float mean, float standardDeviation);

int htw_randIntNormal(int mean, int standardDeviation);

/**
 * @brief Random value with a Gamma probabality distribution
 * Mode of result = a >= 1 ? (a - 1) * b : 0
 * See: https://en.wikipedia.org/wiki/Gamma_distribution
 *
 * @param a shape parameter, must be > 0
 * @param b scale parameter, must be > 0; typically = 1
 * @return float from 0 to RAND_MAX
 */
float htw_randGamma(float a, float b);

/**
 * @brief Random value with a Beta probabality distribution
 * See: https://en.wikipedia.org/wiki/Beta_distribution
 *
 * @param a must be > 0
 * @param b must be > 0
 * @return float from 0 to 1
 */
float htw_randBeta(float a, float b);

/**
 * @brief Random value with a PERT probabality distribution
 * Similar to a normal distribution with easy to control parameters and the ability to skew results towards the low or high end.
 * Must satisfy min < mode < max
 * See: https://en.wikipedia.org/wiki/PERT_distribution
 *
 * @param min roughly 3 standard deviations below the mode
 * @param max roughly 3 standard deviations above the mode
 * @param mode most likely value
 * @return float from min to max
 */
float htw_randPERT(float min, float max, float mode);

/** @}*/

/**
 * @defgroup hash Hash functions
 * xxHash limited implementation for procedural generation. See source file for copyright notice.
 * @{
 */

/**
 * @brief Fash hash of 2 values
 *
 * @param seed optional seed
 * @param x
 * @param y
 * @return seeded hash of x & y
 */
u32 xxh_hash2d(u32 seed, u32 x, u32 y);

/**
 * @brief Fast and well-distributed hash
 * NOTE: if passing an array that isn't u8 or chars, should multiply length by sizeof(type)
 *
 * @param seed optional seed
 * @param length number of bytes from array to digest.
 * @param bytes array of bytes to digest
 * @return seeded hash of bytes
 */
u32 xxh_hash(u32 seed, size_t length, const u8 *bytes);

/** @}*/

/**
 * @defgroup noise 2D Noise
 * @{
 */

//
/**
 * @brief Hermite curve, 3t^2 - 2t^3. Identical to glsl smoothstep in [0, 1]
 * NOTE: for inputs outside [0, 1], this continues in the opposite direction i.e. -x^3
 *
 * @param v from 0 to 1
 * @return float
 */
float htw_smoothCurve(float v);

/**
 * @brief Very similar to a Hermite curve, but with continuous derivative when clamped to [0, 1]
 * NOTE: for inputs outside [0, 1], this continues in the same direction i.e. x^5
 *
 * @param v p_v:...
 * @return float
 */
float htw_smootherCurve(float v);

// Value noise
float htw_value2d(u32 seed, float sampleX, float sampleY);

float htw_value2dRepeating(u32 seed, float sampleX, float sampleY, float repeatX, float repeatY);

// Perlin noise
float htw_perlin2d(u32 seed, float sampleX, float sampleY, u32 octaves);

float htw_perlin2dRepeating(u32 seed, float sampleX, float sampleY, u32 octaves, float repeatX, float repeatY);

/**
 * @brief Very limited simplex noise implementation. Designed for use on a hexagonal tile map with descrete cells, so smoothness/continuous derivites are not a concern, and the input sample coordinates are assumed to already be in a skewed hexmap coordinate space.
 *
 * @param seed p_seed:...
 * @param sampleX p_sampleX:...
 * @param sampleY p_sampleY:...
 * @return float
 */
float htw_simplex2d(u32 seed, float sampleX, float sampleY, u32 repeatX, u32 repeatY);

float htw_simplex2dLayered(u32 seed, float sampleX, float sampleY, u32 repeat, u32 layers);

/** @}*/

#endif // HTW_RANDOM_H_INCLUDED
