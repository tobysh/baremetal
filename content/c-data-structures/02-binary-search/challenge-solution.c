#include <stdio.h>
#include <limits.h>

int first_true(int lo, int hi, int (*pred)(int, void *), void *ctx) {
    while (lo < hi) {
        int mid = lo + (int)(((long)hi - lo) / 2);
        if (pred(mid, ctx)) hi = mid;
        else lo = mid + 1;
    }
    return lo;
}

static int at_least_ten(int i, void *ctx) {
    (void)ctx;
    return i >= 10;
}

// Optional: try it with Run.
int main(void) {
    printf("first_true(0, 100, at_least_ten) = %d (expected 10)\n", first_true(0, 100, at_least_ten, NULL));
    return 0;
}
