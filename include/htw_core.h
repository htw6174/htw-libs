#ifndef HTW_CORE_H_INCLUDED
#define HTW_CORE_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* linux kernal style typedefs */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/* Debug utilities */
#define HTW_STOPWATCH(x) { clock_t stopwatch__start = clock(); \
                    x; \
                    clock_t stopwatch__end = clock(); \
                    float stopwatch__seconds = (float)(stopwatch__end - stopwatch__start) / CLOCKS_PER_SEC; \
                    printf("%s finished in %.3f seconds / %li ticks\n", #x, stopwatch__seconds, stopwatch__end - stopwatch__start); }

static inline void htw_printArray(FILE* dest, void* data, u32 size, u32 count, u32 valuesPerLine, char* format) {
    for (int i = 0; i < count; i++) {
        void* p = (char*)data + (i * size);
        fprintf(dest, format, *(unsigned int*)p);
        // end of line
        if (i % valuesPerLine == (valuesPerLine - 1)) fprintf(dest, "\n");
    }
    // if last line isn't filled, add a newline
    if (count % valuesPerLine != 0) fprintf(dest, "\n");
}

// FIXME: same as printArray right now
static inline void htw_printVectorArray(FILE* dest, void* data, u32 dim, u32 count, u32 vecsPerLine, char* format) {
    for (int i = 0; i < count; i++) {
        void* p = (char*)data + (i * sizeof(float));
        fprintf(dest, format, *(unsigned int*)p);
        // end of line
        if (i % vecsPerLine == (vecsPerLine - 1)) fprintf(dest, "\n");
    }
    // if last line isn't filled, add a newline
    if (count % vecsPerLine != 0) fprintf(dest, "\n");
}

/* String and char utilities */
// Corresponds to the index of 0 in the standard character set
#define INT_CHAR_OFFSET 48
#define charToInt(c) (int)c - INT_CHAR_OFFSET
#define intToChar(i) (char)i + INT_CHAR_OFFSET

static inline int htw_strToInt(const char *str) {
    int sum = 0;
    int i = 0;
    char c;
    while((c = str[i++]) != '\0') {
        sum = (sum * 10) + charToInt(c);
    }
    return sum;
}

/* Math utilities */

#define EULER           2.7182818284590452354f
#define PI              3.14159265358979323846f
#define TAU             6.28318530718f
#define DEG_TO_RAD      0.0174532925199f
#define RAD_TO_DEG      57.2957795131f

/// a % b, but gives the correct result when a is negative
#define MOD(a, b) ((((a)%(b))+(b))%(b))

/// Branchless sign. NOTE: SIGN(0) = 0
#define SIGN(x) (((x) > 0) - ((x) < 0))

/*
 * NOTE: These MIN/MAX definitions are vulnerable to multiple evaluation. Could instead use a compound statement:
 * #define max(a,b) \
 * ( { __typeof__ (a) _a = *(a); \
 * __typeof__ (b) _b = (b); \
 * _a > _b ? _a : _b; })
 *
 * There are 2 issues with this approach however:
 * - __typeof__ is GCC-specific, and widely supported, but not universal
 * - Can cause variable shadowing, which would invalidate the definition of CLAMP below
 */

/// WARNING: multiple evaluation
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
/// WARNING: multiple evaluation
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
/// WARNING: multiple evaluation
#define CLAMP(x, min, max) MAX(min, MIN(x, max))

#define IS_POW_OF_2(x) ((((x) - 1) & (x)) == 0)

/// returns the smallest multiple of alignment which is >= value
int htw_align(int value, int alignment);

/// returns the smallest power of 2 which is >= value
unsigned int htw_nextPow(unsigned int value);

/// unbounded lerp
double lerp(double a, double b, double progress);

/// lerp with result clamped to [a, b]
double lerpClamp(double a, double b, double progress);

/**
 * @brief The progress of value from a to b.
 * If value is between a and b, return is between 0 and 1
 *
 * @param a p_a:...
 * @param b p_b:...
 * @param value p_value:...
 * @return double
 */
double inverseLerp(double a, double b, double value);

int lerp_int(int a, int b, double prog);

double inverseLerp_int(int a, int b, int val);

/**
 * @brief Remap value from one range to another.
 * Same as inverseLerp from old range, then lerp to new range
 *
 * @param value p_value:...
 * @param oldMin p_oldMin:...
 * @param oldMax p_oldMax:...
 * @param newMin p_newMin:...
 * @param newMax p_newMax:...
 * @return double
 */
double remap(double value, double oldMin, double oldMax, double newMin, double newMax);

int remap_int(int val, int oldMin, int oldMax, int newMin, int newMax);

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

// Identical to glsl smoothstep
float htw_smoothstep(float edge0, float edge1, float x);

/// Smoothstep with continuous derivative at x = edge0 and x = edge1
float htw_smootherstep(float edge0, float edge1, float x);

/* Simple file handling utilities */
static const int HTW_FILE_LOAD_MAX_LENGTH = 1024*1024;

// Simply free the result when you're done with it
static char* htw_load(const char* path);

static char* htw_load(const char* path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        return NULL;
    }

    char* contents = calloc(HTW_FILE_LOAD_MAX_LENGTH, sizeof(char));
    size_t i = 0;
    char c;
    while((c = getc(fp)) != EOF) {
        contents[i++] = c;
        if (i == HTW_FILE_LOAD_MAX_LENGTH) {
            contents[i - 1] = 0;
            fprintf(stderr, "Reached max readable length for file %s\n", path);
            break;
        }
    }

    //printf("Loaded %lu bytes from %s\n", i, path);

    fclose(fp);
    return contents;
}

// TODO
/** Generic object pools
 * Allows for frequent reuse of objects without reallocation of memory. Creating a pool allocates enough space for n objects once. The pool keeps track of which pool items are in use/not in use. Requesting a new object from the pool returns the pointer to an unused item, and marks that item as in use. Telling the pool to destroy an object marks it as unused. Destroying the pool frees all items from memory.
 */
typedef struct {
    int capacity;
    int itemSize;
    void *poolItems;
} Pool;

Pool *createPool(int itemSize, int capacity);

/**
 * @brief Free all memory used by Pool [pool]
 *
 * @param pool pool to destroy
 * @return >= 0: number of pool objects still in use. < 0: error
 */
int destroyPool(Pool *pool);
void *getNewPoolItem(Pool *pool);
int destroyPoolItem(Pool *pool, void *item);

#endif // HTW_CORE_H_INCLUDED
