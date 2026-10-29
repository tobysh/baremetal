#include <stdint.h>
#include <stdlib.h>
static long serial_sum(const int *a, int n) {
    long s = 0;
    for (int i = 0; i < n; i++) s += a[i];
    return s;
}

int main(void) {
    int small[6] = {5, -2, 9, 4, 1, 7};
    Range r = {small, 1, 5};
    void *res = sum_worker(&r);
    CHECK("sum_worker returns the sum of a[lo..hi)", (long)(intptr_t)res == 12, "got %ld", (long)(intptr_t)res);

    Range e = {small, 3, 3};
    void *res0 = sum_worker(&e);
    CHECK("sum_worker on an empty range returns 0", (long)(intptr_t)res0 == 0, "got %ld", (long)(intptr_t)res0);
    Range neg = {small, 1, 2};
    small[1] = -2;
    void *resn = sum_worker(&neg);
    CHECK("sum_worker keeps negative sums", (long)(intptr_t)resn == -2, "got %ld", (long)(intptr_t)resn);

    CHECK("parallel_sum with n = 0 is 0", parallel_sum(small, 0, 4) == 0, "expected 0");
    CHECK("parallel_sum with one thread", parallel_sum(small, 6, 1) == 24, "got %ld", parallel_sum(small, 6, 1));
    CHECK("parallel_sum with more threads than items", parallel_sum(small, 6, 8) == 24, "got %ld", parallel_sum(small, 6, 8));
    CHECK("parallel_sum when n does not divide evenly", parallel_sum(small, 5, 4) == 17, "got %ld", parallel_sum(small, 5, 4));

    int n = 100000;
    int *big = malloc(sizeof(int) * (size_t)n);
    for (int i = 0; i < n; i++) big[i] = (i * 37) % 1001 - 400;
    long want = serial_sum(big, n);
    int ok = 1;
    long got = 0;
    for (int k = 1; k <= 8; k++) {
        got = parallel_sum(big, n, k);
        if (got != want) { ok = 0; break; }
    }
    CHECK("parallel_sum of 100000 numbers matches a plain loop for 1 to 8 threads", ok, "expected %ld, got %ld", want, got);
    for (int rep = 0; rep < 30; rep++) if (parallel_sum(big, 1000 + rep, 5) != serial_sum(big, 1000 + rep)) ok = 0;
    CHECK("parallel_sum is right on repeated runs", ok, "a total was wrong");
    free(big);
    return 0;
}
