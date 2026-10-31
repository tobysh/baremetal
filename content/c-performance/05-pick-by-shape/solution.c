#include <stdio.h>
#include <string.h>

#define MAXV 4096

// Comparison counter. Do not edit.
static long ncmp;
static int cmp3(int x, int y) { ncmp++; return x < y ? -1 : x > y; }

// Count the values present in both sorted arrays of distinct ints.
static int ceil_log2(int n) { int k = 0; while ((1L << k) < n) k++; return k; }

static long merge(const int *a, int na, const int *b, int nb) {
    long found = 0;
    int i = 0, j = 0;
    while (i < na && j < nb) {
        int c = cmp3(a[i], b[j]);
        if (c == 0) { found++; i++; j++; }
        else if (c < 0) i++;
        else j++;
    }
    return found;
}

static long search(const int *s, int ns, const int *big, int nbig) {
    long found = 0;
    int lo0 = 0;
    for (int i = 0; i < ns; i++) {
        int lo = lo0, hi = nbig;          // first index with big[idx] >= s[i]
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (cmp3(big[mid], s[i]) < 0) lo = mid + 1; else hi = mid;
        }
        lo0 = lo;
        if (lo < nbig && big[lo] == s[i]) found++;   // equality on the position found, via the order already known
    }
    return found;
}

long count_common(const int *a, int na, const int *b, int nb) {
    if (na > nb) return count_common(b, nb, a, na);
    if (na == 0) return 0;
    if ((long)na * ceil_log2(nb + 1) < (long)na + nb) return search(a, na, b, nb);
    return merge(a, na, b, nb);
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
