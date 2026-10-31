#include <stdio.h>
#include <string.h>

#define MAXV 4096

// Comparison counter. Do not edit.
static long ncmp;
static int cmp3(int x, int y) { ncmp++; return x < y ? -1 : x > y; }

// Count the values present in both sorted arrays of distinct ints.
long count_common(const int *a, int na, const int *b, int nb) {
    long found = 0;
    for (int i = 0; i < na; i++)
        for (int j = 0; j < nb; j++)
            if (cmp3(a[i], b[j]) == 0) found++;
    return found;
}

static int xs[MAXV], ys[MAXV];

// Fills v with n sorted distinct values: start, then steps of 1..step.
static void make(int *v, int n, int start, int step, unsigned seed) {
    unsigned x = seed;
    int cur = start;
    for (int i = 0; i < n; i++) {
        x = x * 1103515245u + 12345u;
        cur += 1 + (int)((x >> 8) % (unsigned)step);
        v[i] = cur;
    }
}

int main(void) {
    int shapes[3][2] = {{2000, 2000}, {1, 4000}, {4000, 40}};
    for (int s = 0; s < 3; s++) {
        make(xs, shapes[s][0], 0, 3, 1);
        make(ys, shapes[s][1], 0, 3, 2);
        ncmp = 0;
        long c = count_common(xs, shapes[s][0], ys, shapes[s][1]);
        printf("na=%4d nb=%4d common=%5ld comparisons=%ld\n", shapes[s][0], shapes[s][1], c, ncmp);
    }
    return 0;
}
