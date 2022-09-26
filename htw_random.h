/* None of the methods here are secure, and should NOT be used for cryptography, passwords, or slot machines
 * If anything, these random number generation methods are optimized for speed and even distribution
 */
#ifndef HTW_RANDOM_H_INCLUDED
#define HTW_RANDOM_H_INCLUDED

#include <math.h>

extern int htw_randRange(int range);

/* Basics */
/**
 * @brief Get a random number in the range [0, range)
 *
 * @param range number of possible results; also equal to max result + 1
 * @return int
 */
inline int htw_randRange(int range) {
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
int htw_rtd(int count, int sides, int *results) {
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

#endif // HTW_RANDOM_H_INCLUDED
