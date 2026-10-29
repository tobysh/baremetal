#include <stdio.h>

int array_sum(int a[], int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += a[i];
    }
    return total;
}

int array_max(int a[], int n) {
    int best = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] > best) best = a[i];
    }
    return best;
}

int count_above(int a[], int n, int limit) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] > limit) count++;
    }
    return count;
}

int main(void) {
    int scores[5] = {70, 85, 90, 60, 100};
    printf("sum = %d\n", array_sum(scores, 5));
    printf("max = %d\n", array_max(scores, 5));
    printf("above 80 = %d\n", count_above(scores, 5, 80));
    return 0;
}
