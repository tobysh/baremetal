#include <stdio.h>
#include <stddef.h>

int array_sum(const int *arr, size_t len) {
    int total = 0;
    for (size_t i = 0; i < len; i++) {
        total += arr[i];
    }
    return total;
}

int array_max(const int *arr, size_t len) {
    int best = arr[0];
    for (size_t i = 1; i < len; i++) {
        if (arr[i] > best) {
            best = arr[i];
        }
    }
    return best;
}

int main(void) {
    int nums[] = {3, -1, 7, 2};
    printf("sum = %d\n", array_sum(nums, 4));
    printf("max = %d\n", array_max(nums, 4));
    return 0;
}
