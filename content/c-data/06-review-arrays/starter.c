#include <stdio.h>

void prefix_sums(int a[], int n, int out[]) {
}

int second_largest(int a[], int n) {
    return 0;
}

int arrays_equal(int a[], int b[], int n) {
    return -1;
}

int count_peaks(int a[], int n) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    int a[5] = {3, 1, 4, 1, 5};
    int out[5];
    prefix_sums(a, 5, out);
    printf("%d %d %d %d %d (expected 3 4 8 9 14)\n", out[0], out[1], out[2], out[3], out[4]);
    printf("second largest = %d (expected 4)\n", second_largest(a, 5));
    printf("peaks = %d (expected 1)\n", count_peaks(a, 5));
    return 0;
}
