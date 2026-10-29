#include <stdio.h>

int array_sum(int a[], int n) {
    return 0;
}

int array_max(int a[], int n) {
    return 0;
}

int count_above(int a[], int n, int limit) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    int scores[5] = {70, 85, 90, 60, 100};
    printf("sum = %d (expected 405)\n", array_sum(scores, 5));
    printf("max = %d (expected 100)\n", array_max(scores, 5));
    printf("above 80 = %d (expected 3)\n", count_above(scores, 5, 80));
    return 0;
}
