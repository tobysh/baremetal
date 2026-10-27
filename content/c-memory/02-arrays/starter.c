#include <stdio.h>
#include <stddef.h>

int array_sum(const int *arr, size_t len) {
    return 0;
}

int array_max(const int *arr, size_t len) {
    return 0;
}

// Optional: try your functions with Run.
int main(void) {
    int nums[] = {3, -1, 7, 2};
    printf("sum = %d (expected 11)\n", array_sum(nums, 4));
    printf("max = %d (expected 7)\n", array_max(nums, 4));
    return 0;
}
