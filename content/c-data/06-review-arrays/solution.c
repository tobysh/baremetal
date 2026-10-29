#include <stdio.h>

void prefix_sums(int a[], int n, int out[]) {
    int running = 0;
    for (int i = 0; i < n; i++) {
        running += a[i];
        out[i] = running;
    }
}

int second_largest(int a[], int n) {
    int best = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] > best) best = a[i];
    }
    int second = 0;
    int have = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] < best && (!have || a[i] > second)) {
            second = a[i];
            have = 1;
        }
    }
    return second;
}

int arrays_equal(int a[], int b[], int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

int count_peaks(int a[], int n) {
    int count = 0;
    for (int i = 1; i + 1 < n; i++) {
        if (a[i] > a[i - 1] && a[i] > a[i + 1]) count++;
    }
    return count;
}

int main(void) {
    int a[5] = {3, 1, 4, 1, 5};
    int out[5];
    prefix_sums(a, 5, out);
    printf("%d %d %d %d %d\n", out[0], out[1], out[2], out[3], out[4]);
    printf("second largest = %d\n", second_largest(a, 5));
    printf("peaks = %d\n", count_peaks(a, 5));
    return 0;
}
