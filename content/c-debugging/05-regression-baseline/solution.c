#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long bm_work;   // deterministic work counter: one unit per step

// Baseline: slow but correct. Values are in -100..100.
int distinct_baseline(const int *a, int n) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        int seen_before = 0;
        for (int j = 0; j < i; j++) {
            if (a[j] == a[i]) { seen_before = 1; break; }
        }
        if (!seen_before) count++;
    }
    return count;
}

// Same behaviour as the baseline, linear work.
int distinct_fast(const int *a, int n) {
    unsigned char seen[201];
    memset(seen, 0, sizeof seen);
    bm_work += sizeof seen;
    int count = 0;
    for (int i = 0; i < n; i++) {
        bm_work++;
        int v = a[i] + 100;
        if (!seen[v]) { seen[v] = 1; count++; }
    }
    return count;
}

int main(void) {
    int cases[][6] = { {1, 2, 2, 3, 3, 3}, {-3, 3, 0, 0, 5, -5}, {7, 7, 7, 7, 7, 7} };
    for (int c = 0; c < 3; c++) {
        bm_work = 0;
        int f = distinct_fast(cases[c], 6);
        long w = bm_work;
        printf("case %d: baseline=%d fast=%d work=%ld\n", c, distinct_baseline(cases[c], 6), f, w);
    }
    printf("empty : baseline=%d fast=%d\n", distinct_baseline(cases[0], 0), distinct_fast(cases[0], 0));
    return 0;
}
