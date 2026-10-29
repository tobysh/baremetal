#include <stdio.h>

int index_of(int a[], int n, int target) {
    return -2;
}

int last_index_of(int a[], int n, int target) {
    return -2;
}

int count_value(int a[], int n, int target) {
    return -1;
}

int is_sorted(int a[], int n) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    int a[6] = {4, 8, 15, 8, 23, 8};
    printf("index_of 8 = %d (expected 1)\n", index_of(a, 6, 8));
    printf("last_index_of 8 = %d (expected 5)\n", last_index_of(a, 6, 8));
    printf("count_value 8 = %d (expected 3)\n", count_value(a, 6, 8));
    printf("is_sorted = %d (expected 0)\n", is_sorted(a, 6));
    return 0;
}
