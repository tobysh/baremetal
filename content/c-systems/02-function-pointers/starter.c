#include <stdio.h>

void map_ints(int *a, int n, int (*f)(int)) {
}

int count_if(const int *a, int n, int (*pred)(int)) {
    return 0;
}

void sort_ints(int *a, int n, int (*cmp)(int, int)) {
}

// Optional: try them with Run.
static int double_it(int x) { return x * 2; }
static int ascending(int a, int b) { return a - b; }

int main(void) {
    int nums[] = {5, 2, 9, 1};
    map_ints(nums, 4, double_it);
    sort_ints(nums, 4, ascending);
    for (int i = 0; i < 4; i++) printf("%d ", nums[i]);
    printf("\n");
    return 0;
}
