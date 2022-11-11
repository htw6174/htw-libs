/* None of the methods here are secure, and should NOT be used for cryptography, passwords, or slot machines
 * If anything, these random number generation methods are optimized for speed and even distribution
 */
#ifndef HTW_RANDOM_H_INCLUDED
#define HTW_RANDOM_H_INCLUDED

#include <math.h>
#include "htw_core.h"

//extern int htw_randRange(int range);

/* Basics */
/**
 * @brief Get a random number in the range [0, range)
 *
 * @param range number of possible results; also equal to max result + 1
 * @return int
 */
static inline int htw_randRange(int range) {
    return rand() % range;
}

/**
 * @brief Roll The Dice; get total and/or individual results for [count] dice rolls. Result for each roll will be in the range [1, sides]
 *
 * @param count number of dice to roll
 * @param sides number of sides on each die
 * @param results NULL or a pointer to an array of at least [count] elements, to record the result of each roll
 * @return sum of all rolls
 */
static int htw_rtd(int count, int sides, int *results) {
    int total = 0;
    if (results == NULL) {
        for (int i = 0; i < count; i++) {
            total += htw_randRange(sides) + 1;
        }
    }
    else {
        for (int i = 0; i < count; i++) {
            int r = htw_randRange(sides) + 1;
            results[i] = r;
            total += r;
        }
    }
    return total;
}

/* Hash functions */
/* xxHash limited implementation for procedural generation
 *
 * Derived from Rune Skovbo Johansen's modified version, read his post on hashing for random numbers here: https://blog.runevision.com/2015/01/primer-on-repeatable-random-numbers.html
 * The following copyright notice applies to everything with a "XXH_*" identifier in this project:
 * Based on integer-optimized implementation Copyright (C) 2015, Rune Skovbo Johansen. (https://bitbucket.org/runevision/random-numbers-testing/)
 * Based on C# implementation Copyright (C) 2014, Seok-Ju, Yun. (https://github.com/noricube/xxHashSharp)
 * Original C Implementation Copyright (C) 2012-2021 Yann Collet (https://code.google.com/p/xxhash/)
 *
 * BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You can contact the author at:
 *   - xxHash homepage: https://www.xxhash.com
 *   - xxHash source repository: https://github.com/Cyan4973/xxHash
 *
 *
 */

#define XXH_PRIME32_1  0x9E3779B1U  /*!< 0b10011110001101110111100110110001 */
#define XXH_PRIME32_2  0x85EBCA77U  /*!< 0b10000101111010111100101001110111 */
#define XXH_PRIME32_3  0xC2B2AE3DU  /*!< 0b11000010101100101010111000111101 */
#define XXH_PRIME32_4  0x27D4EB2FU  /*!< 0b00100111110101001110101100101111 */
#define XXH_PRIME32_5  0x165667B1U  /*!< 0b00010110010101100110011110110001 */

static u32 xxh_rotateLeft(u32 value, s32 count) {
    return (value << count) | (value >> (32 - count));
}

static u32 xxh_hash2d(u32 seed, u32 x, u32 y) {
    u32 hash = seed + XXH_PRIME32_5;
    hash += 2 * 4; // equivalent to adding input bytecount in the original. Unsure if it's needed here
    // unrolled loop for fixed length input
    // iter 1:
    hash += x * XXH_PRIME32_3;
    hash = xxh_rotateLeft(hash, 17) * XXH_PRIME32_4;
    // iter 2:
    hash += y * XXH_PRIME32_3;
    hash = xxh_rotateLeft(hash, 17) * XXH_PRIME32_4;

    hash ^= hash >> 15;
    hash *= XXH_PRIME32_2;
    hash ^= hash >> 13;
    hash *= XXH_PRIME32_3;
    hash ^= hash >> 16;

    return hash;
}

/* 2D Noise */

// Value noise
static float htw_value2d(u32 seed, float sampleX, float sampleY) {
    float integralX, integralY;
    float fractX = modff(sampleX, &integralX);
    float fractY = modff(sampleY, &integralY);

    // samples
    float s1 = (float)xxh_hash2d(seed, integralX, integralY);
    float s2 = (float)xxh_hash2d(seed, integralX + 1, integralY);
    float s3 = (float)xxh_hash2d(seed, integralX, integralY + 1);
    float s4 = (float)xxh_hash2d(seed, integralX + 1, integralY + 1);

    float l1 = lerp(s1, s2, fractX);
    float l2 = lerp(s3, s4, fractX);
    float l3 = lerp(l1, l2, fractY);

    float normalized = l3 / (float)UINT32_MAX;
    return normalized;
}

// Perlin noise
static float htw_perlin2d(u32 seed, float sampleX, float sampleY, u32 octaves) {
    u32 numerator = pow(2, octaves - 1); // used to weight each octave by half the previous octave, halves each iteration
    u32 denominator = pow(2, octaves) - 1;
    float value = 0.0;
    float scaledX = sampleX;
    float scaledY = sampleY;
    for (int i = 0; i < octaves; i++) {
        float weight = (float)numerator / denominator;
        numerator = numerator >> 1;
        value += htw_value2d(seed, scaledX, scaledY) * weight;
        scaledX *= 2.0;
        scaledY *= 2.0;
    }
    return value;
}

#endif // HTW_RANDOM_H_INCLUDED
