#include <stdio.h>
#include <math.h>
#include "htw_core.h"
#include "htw_random.h"

/** TODO: assert macros (or functions, if it works) should display:
 * - a description of the condition that failed
 * - names and values of the arguments submitted to the condition
 * - maybe outside of scope, but would be nice to include names and values of every part of current test's state
 *
 * Option 1: during each test, construct string containing all state details, pass to assert, and print if assert fails
 * Option 2: Create a runtime exception if assert fails. This way, a debugger can inspect current state before terminating (stupid enough to work?)
 *
 */

#define ASSERT_TRUE(a) if (a == 0) exit(-1)
#define ASSERT_FALSE(a) if (a != 0) exit(-1)
#define ASSERT_EQUAL(a, b) if (a != b) fprintf(stderr, "Assert equal failed: %i != %i\n", a, b);
#define ASSERT_GT(a, b) if (a <= b) fprintf(stderr, "Assert greater than failed: %i <= %i\n", a, b);
#define ASSERT_LT(a, b) if (a >= b) fprintf(stderr, "Assert less than failed: %s >= %s\n", #a, #b);
#define ASSERT_NOT_EQUAL(a, b) if (a == b) fprintf(stderr, "Assert not equal failed: %s == %s\n", #a, #b);

char testConditions[1000];

void printBuckets(int num_buckets, int num_trials, float min, float max, int *buckets) {
    printf("| bucket range | count | percent of total | chance to match or beat\n");
    double cumulativeOdds[num_buckets];
    for (int i = 0; i < num_buckets; i++) {
        double p = ((double)buckets[i] / num_trials) * 100;
        if (i > 0) cumulativeOdds[i] = cumulativeOdds[i - 1] + p;
        else cumulativeOdds[0] = p;
        printf("| %6.2f to %6.2f | %7d | %5.2f%% | %6.2f%%\n",
               (float)i     * (max - min) / num_buckets + min,
               (float)(i+1) * (max - min) / num_buckets + min,
               buckets[i],
               p,
               100.0 - cumulativeOdds[i] + p
        );
    }
}

int test_core() {
    return 0;
}

int test_randInt() {
    for (int i = 1; i < 20; i++) {
        int r = htw_randInt(i);
        sprintf(testConditions, "range = %i", i);
        ASSERT_GT(r, -1);
        ASSERT_LT(r, i + 1);
    }
    return 0;
}

int test_rtd() {
    for (int d = 0; d < 20; d++) {
        for (int s = 1; s < 20; s++) {
            int t = htw_rtd(d, s, NULL);
            //ASSERT(d <= t <= (d * s));
            ASSERT_GT(t, d - 1);
            ASSERT_LT(t, (d * s) + 1);
        }
    }
    return 0;
}

void test_rtd_histogram(int d, int s, int n) {
    printf("Rolling %id%i:\n", d, s);
    int min = d;
    int max = d * s;
    int range = (max - min) + 1;
    int results[range];
    for (int i = 0; i < range; i++) {
        results[i] = 0;
    }

    for (int i = 0; i < n; i++) {
        int t = htw_rtd(d, s, NULL);
        results[t - min] += 1;
    }

    printf("| result | percent of total | chance to match or beat | total\n");
    double cumulativeOdds[range];
    for (int i = 0; i < range; i++) {
        double p = ((double)results[i] / n) * 100;
        if (i > 0) cumulativeOdds[i] = cumulativeOdds[i - 1] + p;
        else cumulativeOdds[0] = p;
        printf("| %2i | %5.2f%% | %6.2f%% | %d\n", i + min, p, 100.0 - cumulativeOdds[i] + p, results[i]);
    }
}

void test_randPERT() {
    const float min = -50;
    const float max = 50;
    const float mode = -25;

    const int num_buckets = 12;
    int buckets[num_buckets];
    for (int i = 0; i < num_buckets; i++) {
        buckets[i] = 0;
    }

    const int num_trials = 1000000;
    //float results[num_trials];
    for (int i = 0; i < num_trials; i++) {
        float result = htw_randPERT(min, max, mode);
        int bucket_index = floorf(remap(result, min, max, 0, num_buckets));
        ++buckets[bucket_index];
        //results[i] = result;
    }

    printBuckets(num_buckets, num_trials, min, max, buckets);
}

int test_random() {
    int failures = 0;
    failures += test_randInt();
    failures += test_rtd();
    //test_rtd_histogram(3, 6, 10000000);
    //test_rtd_histogram(1, 20, 10000000);
    test_randPERT();
    return failures;
}

int main(int argc, char* argv[]) {
    int failures = 0;
    failures += test_core();
    failures += test_random();
    printf("All tests completed. Failures: %i\n", failures);
}
