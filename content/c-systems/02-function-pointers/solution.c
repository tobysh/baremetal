#include <stdio.h>

void map_ints(int *a, int n, int (*f)(int)) {
    for (int i = 0; i < n; i++) {
        a[i] = f(a[i]);
    }
}

int count_if(const int *a, int n, int (*pred)(int)) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (pred(a[i])) {
            count++;
        }
    }
    return count;
}

void sort_ints(int *a, int n, int (*cmp)(int, int)) {
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && cmp(a[j], key) > 0) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

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
