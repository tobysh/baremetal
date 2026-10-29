#include <stdio.h>
#include <limits.h>

int first_true(int lo, int hi, int (*pred)(int, void *), void *ctx) {
    return hi;
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
