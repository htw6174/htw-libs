#include "htw_core.h"

int min_int(int a, int b);

int max_int(int a, int b);

int lerp_int(int a, int b, double prog) {
    if (prog > 1) return b;
    if (prog < 0) return a;
    int range = b - a;
    return a + (prog * range);
}

double inverseLerp_int(int a, int b, int val) {
    if (val > b) return 1;
    if (val < a) return 0;
    return (double)(val - a) / (b - a);
}

int remap_int(int val, int oldMin, int oldMax, int newMin, int newMax) {
    double prog = inverseLerp_int(oldMin, oldMax, val);
    return lerp_int(newMin, newMax, prog);
}
