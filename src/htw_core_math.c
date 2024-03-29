#include "htw_core.h"
#include <math.h>

// NOTE: the <sys/param.h> header contains some other useful macros, but unsure if they are portable

int htw_align(int value, int alignment) {
    int aligned = value; // return same size if it fits cleanly
    int diff = value % alignment;
    if (diff != 0) {
        // if there is an uneven remainder, increase to the next multiple of alignment
        aligned = value + (alignment - diff);
    }
    return aligned;
}

unsigned int htw_nextPow(unsigned int value) {
    if IS_POW_OF_2(value) {
        return value;
    } else {
        return ceil(log2(value));
    }
}

double lerp(double a, double b, double progress) {
    return (a * (1.0 - progress)) + (b * progress);
}

double lerpClamp(double a, double b, double progress) {
    if (progress < 0.0) return a;
    if (progress > 1.0) return b;
    return (a * (1.0 - progress)) + (b * progress);
}

double inverseLerp(double a, double b, double value) {
    return (value - a) / (b - a);
}

int lerp_int(int a, int b, double prog) {
    if (prog < 0) return a;
    if (prog > 1) return b;
    int range = b - a;
    return a + (prog * range);
}

double inverseLerp_int(int a, int b, int val) {
    if (val < a) return 0;
    if (val > b) return 1;
    return (double)(val - a) / (b - a);
}

double remap(double value, double oldMin, double oldMax, double newMin, double newMax) {
    double normalized = (value - oldMin) / (oldMax - oldMin);
    return newMin + (normalized * (newMax - newMin));
}

int remap_int(int val, int oldMin, int oldMax, int newMin, int newMax) {

    double prog = inverseLerp_int(oldMin, oldMax, val);
    return lerp_int(newMin, newMax, prog);
}

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

float htw_smoothstep(float edge0, float edge1, float x) {
    x = (x - edge0) / (edge1 - edge0);
    x = CLAMP(x, 0.0, 1.0);
    return htw_smoothCurve(x);
}

float htw_smootherstep(float edge0, float edge1, float x) {
    x = (x - edge0) / (edge1 - edge0);
    x = CLAMP(x, 0.0, 1.0);
    return htw_smootherCurve(x);
}
