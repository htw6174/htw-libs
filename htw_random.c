#include "htw_random.h"
#include "htw_core.h"
#include <math.h>

//extern int htw_randRange(int range);

/* Basics */

int htw_randInt(int max) {
    return rand() % (max + 1);
}

int htw_randIndex(int size) {
    return rand() % size;
}

int htw_randIntRange(int min, int max) {
    return (rand() % ((max - min) + 1)) + min;
}

float htw_randValue() {
    return (float)rand() / (float)RAND_MAX;
}

float htw_randRange(float min, float max) {
    return fmaf((float)rand() / (float)RAND_MAX, max - min, min);
}

int htw_coinFlip() {
    return rand() & 1;
}

int htw_weightedFlip(float p) {
    return htw_randValue() < p;
}


int htw_rtd(int count, int sides, int *results) {
    int total = 0;
    if (results == NULL) {
        for (int i = 0; i < count; i++) {
            total += htw_randInt(sides) + 1;
        }
    }
    else {
        for (int i = 0; i < count; i++) {
            int r = htw_randInt(sides) + 1;
            results[i] = r;
            total += r;
        }
    }
    return total;
}

/* htw_randGamma and gsl_ran_gaussian_ziggurat and supporting constants are adapted from GSL, the GNU Scientific Library.
 * License follows:
 *
 * gauss.c - gaussian random numbers, using the Ziggurat method
 *
 * Copyright (C) 2005  Jochen Voss.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * This routine is based on the following article, with a couple of
 * modifications which simplify the implementation.
 *
 *     George Marsaglia, Wai Wan Tsang
 *     The Ziggurat Method for Generating Random Variables
 *     Journal of Statistical Software, vol. 5 (2000), no. 8
 *     http://www.jstatsoft.org/v05/i08/
 *
 * The modifications are:
 *
 * 1) use 128 steps instead of 256 to decrease the amount of static
 * data necessary.
 *
 * 2) use an acceptance sampling from an exponential wedge
 * exp(-R*(x-R/2)) for the tail of the base strip to simplify the
 * implementation.  The area of exponential wedge is used in
 * calculating 'v' and the coefficients in ziggurat table, so the
 * coefficients differ slightly from those in the Marsaglia and Tsang
 * paper.
 *
 * See also Leong et al, "A Comment on the Implementation of the
 * Ziggurat Method", Journal of Statistical Software, vol 5 (2005), no 7.
 *
 */

/* position of right-most step */
#define PARAM_R 3.44428647676

/* tabulated values for the heigt of the Ziggurat levels */
static const double ytab[128] = {
    1, 0.963598623011, 0.936280813353, 0.913041104253,
    0.892278506696, 0.873239356919, 0.855496407634, 0.838778928349,
    0.822902083699, 0.807732738234, 0.793171045519, 0.779139726505,
    0.765577436082, 0.752434456248, 0.739669787677, 0.727249120285,
    0.715143377413, 0.703327646455, 0.691780377035, 0.68048276891,
    0.669418297233, 0.65857233912, 0.647931876189, 0.637485254896,
    0.62722199145, 0.617132611532, 0.607208517467, 0.597441877296,
    0.587825531465, 0.578352913803, 0.569017984198, 0.559815170911,
    0.550739320877, 0.541785656682, 0.532949739145, 0.524227434628,
    0.515614886373, 0.507108489253, 0.498704867478, 0.490400854812,
    0.482193476986, 0.47407993601, 0.466057596125, 0.458123971214,
    0.450276713467, 0.442513603171, 0.434832539473, 0.427231532022,
    0.419708693379, 0.41226223212, 0.404890446548, 0.397591718955,
    0.390364510382, 0.383207355816, 0.376118859788, 0.369097692334,
    0.362142585282, 0.355252328834, 0.348425768415, 0.341661801776,
    0.334959376311, 0.328317486588, 0.321735172063, 0.31521151497,
    0.308745638367, 0.302336704338, 0.29598391232, 0.289686497571,
    0.283443729739, 0.27725491156, 0.271119377649, 0.265036493387,
    0.259005653912, 0.253026283183, 0.247097833139, 0.241219782932,
    0.235391638239, 0.229612930649, 0.223883217122, 0.218202079518,
    0.212569124201, 0.206983981709, 0.201446306496, 0.195955776745,
    0.190512094256, 0.185114984406, 0.179764196185, 0.174459502324,
    0.169200699492, 0.1639876086, 0.158820075195, 0.153697969964,
    0.148621189348, 0.143589656295, 0.138603321143, 0.133662162669,
    0.128766189309, 0.123915440582, 0.119109988745, 0.114349940703,
    0.10963544023, 0.104966670533, 0.100343857232, 0.0957672718266,
    0.0912372357329, 0.0867541250127, 0.082318375932, 0.0779304915295,
    0.0735910494266, 0.0693007111742, 0.065060233529, 0.0608704821745,
    0.056732448584, 0.05264727098, 0.0486162607163, 0.0446409359769,
    0.0407230655415, 0.0368647267386, 0.0330683839378, 0.0293369977411,
    0.0256741818288, 0.0220844372634, 0.0185735200577, 0.0151490552854,
    0.0118216532614, 0.00860719483079, 0.00553245272614, 0.00265435214565
};

/* tabulated values for 2^24 times x[i]/x[i+1],
 * used to accept for U*x[i+1]<=x[i] without any floating point operations */
static const unsigned long ktab[128] = {
    0, 12590644, 14272653, 14988939,
    15384584, 15635009, 15807561, 15933577,
    16029594, 16105155, 16166147, 16216399,
    16258508, 16294295, 16325078, 16351831,
    16375291, 16396026, 16414479, 16431002,
    16445880, 16459343, 16471578, 16482744,
    16492970, 16502368, 16511031, 16519039,
    16526459, 16533352, 16539769, 16545755,
    16551348, 16556584, 16561493, 16566101,
    16570433, 16574511, 16578353, 16581977,
    16585398, 16588629, 16591685, 16594575,
    16597311, 16599901, 16602354, 16604679,
    16606881, 16608968, 16610945, 16612818,
    16614592, 16616272, 16617861, 16619363,
    16620782, 16622121, 16623383, 16624570,
    16625685, 16626730, 16627708, 16628619,
    16629465, 16630248, 16630969, 16631628,
    16632228, 16632768, 16633248, 16633671,
    16634034, 16634340, 16634586, 16634774,
    16634903, 16634972, 16634980, 16634926,
    16634810, 16634628, 16634381, 16634066,
    16633680, 16633222, 16632688, 16632075,
    16631380, 16630598, 16629726, 16628757,
    16627686, 16626507, 16625212, 16623794,
    16622243, 16620548, 16618698, 16616679,
    16614476, 16612071, 16609444, 16606571,
    16603425, 16599973, 16596178, 16591995,
    16587369, 16582237, 16576520, 16570120,
    16562917, 16554758, 16545450, 16534739,
    16522287, 16507638, 16490152, 16468907,
    16442518, 16408804, 16364095, 16301683,
    16207738, 16047994, 15704248, 15472926
};

/* tabulated values of 2^{-24}*x[i] */
static const double wtab[128] = {
    1.62318314817e-08, 2.16291505214e-08, 2.54246305087e-08, 2.84579525938e-08,
    3.10340022482e-08, 3.33011726243e-08, 3.53439060345e-08, 3.72152672658e-08,
    3.8950989572e-08, 4.05763964764e-08, 4.21101548915e-08, 4.35664624904e-08,
    4.49563968336e-08, 4.62887864029e-08, 4.75707945735e-08, 4.88083237257e-08,
    5.00063025384e-08, 5.11688950428e-08, 5.22996558616e-08, 5.34016475624e-08,
    5.44775307871e-08, 5.55296344581e-08, 5.65600111659e-08, 5.75704813695e-08,
    5.85626690412e-08, 5.95380306862e-08, 6.04978791776e-08, 6.14434034901e-08,
    6.23756851626e-08, 6.32957121259e-08, 6.42043903937e-08, 6.51025540077e-08,
    6.59909735447e-08, 6.68703634341e-08, 6.77413882848e-08, 6.8604668381e-08,
    6.94607844804e-08, 7.03102820203e-08, 7.11536748229e-08, 7.1991448372e-08,
    7.2824062723e-08, 7.36519550992e-08, 7.44755422158e-08, 7.52952223703e-08,
    7.61113773308e-08, 7.69243740467e-08, 7.77345662086e-08, 7.85422956743e-08,
    7.93478937793e-08, 8.01516825471e-08, 8.09539758128e-08, 8.17550802699e-08,
    8.25552964535e-08, 8.33549196661e-08, 8.41542408569e-08, 8.49535474601e-08,
    8.57531242006e-08, 8.65532538723e-08, 8.73542180955e-08, 8.8156298059e-08,
    8.89597752521e-08, 8.97649321908e-08, 9.05720531451e-08, 9.138142487e-08,
    9.21933373471e-08, 9.30080845407e-08, 9.38259651738e-08, 9.46472835298e-08,
    9.54723502847e-08, 9.63014833769e-08, 9.71350089201e-08, 9.79732621669e-08,
    9.88165885297e-08, 9.96653446693e-08, 1.00519899658e-07, 1.0138063623e-07,
    1.02247952126e-07, 1.03122261554e-07, 1.04003996769e-07, 1.04893609795e-07,
    1.05791574313e-07, 1.06698387725e-07, 1.07614573423e-07, 1.08540683296e-07,
    1.09477300508e-07, 1.1042504257e-07, 1.11384564771e-07, 1.12356564007e-07,
    1.13341783071e-07, 1.14341015475e-07, 1.15355110887e-07, 1.16384981291e-07,
    1.17431607977e-07, 1.18496049514e-07, 1.19579450872e-07, 1.20683053909e-07,
    1.21808209468e-07, 1.2295639141e-07, 1.24129212952e-07, 1.25328445797e-07,
    1.26556042658e-07, 1.27814163916e-07, 1.29105209375e-07, 1.30431856341e-07,
    1.31797105598e-07, 1.3320433736e-07, 1.34657379914e-07, 1.36160594606e-07,
    1.37718982103e-07, 1.39338316679e-07, 1.41025317971e-07, 1.42787873535e-07,
    1.44635331499e-07, 1.4657889173e-07, 1.48632138436e-07, 1.50811780719e-07,
    1.53138707402e-07, 1.55639532047e-07, 1.58348931426e-07, 1.61313325908e-07,
    1.64596952856e-07, 1.68292495203e-07, 1.72541128694e-07, 1.77574279496e-07,
    1.83813550477e-07, 1.92166040885e-07, 2.05295471952e-07, 2.22600839893e-07
};

float gsl_ran_gaussian_ziggurat(float sigma) {
    unsigned long int i, j;
    int sign;
    double x, y;

    const unsigned long int range = RAND_MAX;

    while (1)
    {
        if (range >= 0x00FFFFFF)
        {
            unsigned long int k1 = rand();
            unsigned long int k2 = rand();
            i = (k1 & 0xFF);
            j = (k2 & 0x00FFFFFF);
        }
        else
        {
            i = htw_randIndex(256); /*  choose the step */
            j = htw_randIndex(16777216);  /* sample from 2^24 */
        }

        sign = (i & 0x80) ? +1 : -1;
        i &= 0x7f;

        x = j * wtab[i];

        if (j < ktab[i])
            break;

        if (i < 127)
        {
            double y0, y1, U1;
            y0 = ytab[i];
            y1 = ytab[i + 1];
            U1 = htw_randValue();
            y = y1 + (y0 - y1) * U1;
        }
        else
        {
            double U1, U2;
            U1 = 1.0 - htw_randValue();
            U2 = htw_randValue();
            x = PARAM_R - log (U1) / PARAM_R;
            y = exp (-PARAM_R * (x - 0.5 * PARAM_R)) * U2;
        }

        if (y < exp (-0.5 * x * x))
            break;
    }

    return sign * sigma * x;
}

/*
 * Implementation based on the Marsaglia-Tsang method used in GSL.
 * TODO: use guaranteed nonzero rand function where required? At first glance doesn't look like it matters.
 */
float htw_randGamma(float a, float b) {
    /* assume a > 0 */

    if (a < 1)
    {
        double u = htw_randValue();
        return htw_randGamma(1.0 + a, b) * pow (u, 1.0 / a);
    }

    {
        double x, v, u;
        double d = a - 1.0 / 3.0;
        double c = (1.0 / 3.0) / sqrt (d);

        while (1)
        {
            do
            {
                x = gsl_ran_gaussian_ziggurat(1.0);
                v = 1.0 + c * x;
            }
            while (v <= 0);

            v = v * v * v;
            u = htw_randValue();

            if (u < 1 - 0.0331 * x * x * x * x)
                break;

            if (log (u) < 0.5 * x * x + d * (1 - v + log (v)))
                break;
        }

        return b * d * v;
    }
}

float htw_randBeta(float a, float b) {
    // NOTE: Knuth's method is faster when both a and b <= 1, but for my purposes this is an uncommon case.
    float x1 = htw_randGamma(a, 1.0);
    float x2 = htw_randGamma(b, 1.0);
    return x1 / (x1 + x2);
}

float htw_randPERT(float min, float max, float mode) {
    const float LAMBDA = 4.0;
    float ba = mode - min;
    float ca = max - min;
    float cb = max - mode;
    float alpha = fmaf(ba/ca, LAMBDA, 1.0);
    float beta = fmaf(cb/ca, LAMBDA, 1.0);
    return fmaf(htw_randBeta(alpha, beta), ca, min);
}

/* Hash functions */
/* xxHash limited implementation for procedural generation
 *
 * Derived from Rune Skovbo Johansen's modified version, read his post on hashing for random numbers here: https://blog.runevision.com/2015/01/primer-on-repeatable-random-numbers.html
 * The following copyright notice applies to everything with the "xxh" prefix in this file:
 *
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

/// internal
u32 xxh_rotateLeft(u32 value, s32 count) {
    return (value << count) | (value >> (32 - count));
}

/// internal
inline u32 xxh_bytesToU32(const u8 *bytes) {
    u32 value = *bytes;
    value |= (u32)(*(bytes + 1)) << 8;
    value |= (u32)(*(bytes + 2)) << 16;
    value |= (u32)(*(bytes + 3)) << 24;
    return value;
}

/// internal
inline u32 xxh_subHash(u32 value, const u8 *bytes) {
    u32 word = xxh_bytesToU32(bytes);
    value += word * XXH_PRIME32_2;
    value = xxh_rotateLeft(value, 13);
    value *= XXH_PRIME32_1;
    return value;
}

u32 xxh_hash2d(u32 seed, u32 x, u32 y) {
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

u32 xxh_hash(u32 seed, size_t size, const u8 *bytes) {
    u32 hash = 0;
    u32 i = 0;

    if (size >= 16) {
        u32 limit = size - 16;
        u32 v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
        u32 v2 = seed + XXH_PRIME32_2;
        u32 v3 = seed + 0;
        u32 v4 = seed + XXH_PRIME32_1;

        do {
            v1 = xxh_subHash(v1, &bytes[i]);
            i += 4;
            v2 = xxh_subHash(v2, &bytes[i]);
            i += 4;
            v3 = xxh_subHash(v3, &bytes[i]);
            i += 4;
            v4 = xxh_subHash(v4, &bytes[i]);
            i += 4;
        } while (i <= limit);

        hash = xxh_rotateLeft(v1, 1) + xxh_rotateLeft(v2, 7) + xxh_rotateLeft(v3, 12) + xxh_rotateLeft(v4, 18);
    } else {
        hash = seed + XXH_PRIME32_5;
    }

    hash += size;

    while (i <= size - 4) {
        u32 word = xxh_bytesToU32(&bytes[i]);
        hash += word * XXH_PRIME32_3;
        hash = xxh_rotateLeft(hash, 17) * XXH_PRIME32_4;
        i += 4;
    }

    while (i < size) {
        hash += bytes[i] * XXH_PRIME32_5;
        hash = xxh_rotateLeft(hash, 11) * XXH_PRIME32_1;
        i++;
    }

    hash ^= hash >> 15;
    hash *= XXH_PRIME32_2;
    hash ^= hash >> 13;
    hash *= XXH_PRIME32_3;
    hash ^= hash >> 16;

    return hash;
}

/* 2D Noise */

float htw_smoothCurve(float v) {
    return v * v * (3.0 - 2.0 * v);
}

float htw_smootherCurve(float v) {
    return v * v * v * (v * (v * 6.0 - 15.0) + 10.0);
}

// Not sure where this one came from or what it has to do with surflets!
// Similar to 1 - smoothstep() between 0 and 0.7, then increases sharply through and past f(1) = 1
float private_htw_surfletCurve(float v) {
    v = 0.5 - (v * v);
    return v * v * v * v * 16.0;
}

float htw_value2d(u32 seed, float sampleX, float sampleY) {
    float integralX, integralY;
    float fractX = modff(sampleX, &integralX);
    float fractY = modff(sampleY, &integralY);
    u32 x = (u32)integralX, y = (u32)integralY;

    // samples
    float s1 = (float)xxh_hash2d(seed, x, y);
    float s2 = (float)xxh_hash2d(seed, x + 1, y);
    float s3 = (float)xxh_hash2d(seed, x, y + 1);
    float s4 = (float)xxh_hash2d(seed, x + 1, y + 1);

    float l1 = lerp(s1, s2, fractX);
    float l2 = lerp(s3, s4, fractX);
    float l3 = lerp(l1, l2, fractY);

    float normalized = l3 / (float)UINT32_MAX;
    return normalized;
}

float htw_value2dRepeating(u32 seed, float sampleX, float sampleY, float repeatX, float repeatY) {
    float wrappedX = sampleX - (repeatX * floorf(sampleX / repeatX));
    float wrappedY = sampleY - (repeatY * floorf(sampleY / repeatY));
    return htw_value2d(seed, wrappedX, wrappedY);
}

float htw_perlin2d(u32 seed, float sampleX, float sampleY, u32 octaves) {
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

float htw_perlin2dRepeating(u32 seed, float sampleX, float sampleY, u32 octaves, float repeatX, float repeatY) {
    u32 numerator = pow(2, octaves - 1); // used to weight each octave by half the previous octave, halves each iteration
    u32 denominator = pow(2, octaves) - 1;
    float value = 0.0;
    float scaledX = sampleX;
    float scaledY = sampleY;
    for (int i = 0; i < octaves; i++) {
        float weight = (float)numerator / denominator;
        numerator = numerator >> 1;
        value += htw_value2dRepeating(seed, scaledX, scaledY, repeatX, repeatY) * weight;
        scaledX *= 2.0;
        scaledY *= 2.0;
    }
    return value;
}

float htw_simplex2d(u32 seed, float sampleX, float sampleY, u32 repeatX, u32 repeatY) {
    float integralX, integralY;
    float fractX = modff(sampleX, &integralX);
    float fractY = modff(sampleY, &integralY);
    u32 x = (u32)integralX, y = (u32)integralY;

    // The skewing in my system is different than a typical simplex noise impelmentation (angle between x and y axis is 60 instead of 120), so determing which simplex a sample lies in is fractX + fractY < 1, instead of x > y
    u32 simplex = fractX + fractY < 1 ? 0 : 1;

    // wrap sample coordinates
    u32 x0 = x % repeatX;
    u32 x1 = (x + 1) % repeatX;
    u32 y0 = y % repeatY;
    u32 y1 = (y + 1) % repeatY;

    // kernels - deterministic random values at closest simplex corners
    float k1 = (float)xxh_hash2d(seed, x1, y0) / (float)UINT32_MAX;
    float k2 = (float)xxh_hash2d(seed, x0, y1) / (float)UINT32_MAX;
    u32 k3raw = simplex == 0 ? xxh_hash2d(seed, x0, y0) : xxh_hash2d(seed, x1, y1);
    float k3 = (float)k3raw / (float)UINT32_MAX;

    // distance of sample from each corner, using cube coordinate distance
    // = (abs(x1 - x2) + abs(x1 + y1 - x2 - y2) + abs(y1 - y2)) / 2
    float d1 = (fabs(fractX - 1.0) + fabs(fractX + fractY - 1.0 - 0.0) + fabs(fractY - 0.0)) / 2.0;
    float d2 = (fabs(fractX - 0.0) + fabs(fractX + fractY - 0.0 - 1.0) + fabs(fractY - 1.0)) / 2.0;
    float d3 = (fabs(fractX - simplex) + fabs(fractX + fractY - simplex - simplex) + fabs(fractY - simplex)) / 2.0;

    // TODO: could benefit from a non-linear surflet dropoff function or other smoothing, but adding more octaves is good enough for now
    // remap and smooth distance
    // d1 = private_htw_smoothCurve(fmin(d1, 1.0));
    // d2 = private_htw_smoothCurve(fmin(d2, 1.0));
    // d3 = private_htw_smoothCurve(fmin(d3, 1.0));

    // d1 = private_htw_surfletCurve(d1);
    // d2 = private_htw_surfletCurve(d2);
    // d3 = private_htw_surfletCurve(d3);

    // kernel contribution from each corner
    float c1 = k1 * (1.0 - d1);
    float c2 = k2 * (1.0 - d2);
    float c3 = k3 * (1.0 - d3);

    float sum = (c1 + c2 + c3);

    return sum;
}

float htw_simplex2dLayered(u32 seed, float sampleX, float sampleY, u32 repeat, u32 layers) {
    u32 numerator = pow(2, layers - 1); // used to weight each layer by half the previous layer, halves each iteration
    u32 denominator = pow(2, layers) - 1;
    float value = 0.0;
    float scaledX = sampleX;
    float scaledY = sampleY;
    for (int i = 0; i < layers; i++) {
        float weight = (float)numerator / denominator;
        numerator = numerator >> 1;
        value += htw_simplex2d(seed, scaledX, scaledY, repeat, repeat) * weight;
        scaledX *= 2.0;
        scaledY *= 2.0;
        repeat *= 2;
    }
    return value;
}
