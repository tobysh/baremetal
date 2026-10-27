#include <stdio.h>

void swap(int *a, int *b) {
    // Exchange the values that a and b point to.
}

// Optional: try it with Run.
int main(void) {
    int x = 1, y = 2;
    swap(&x, &y);
    printf("x = %d, y = %d (expected x = 2, y = 1)\n", x, y);
    return 0;
}
