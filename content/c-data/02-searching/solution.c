#include <stdio.h>

int index_of(int a[], int n, int target) {
    for (int i = 0; i < n; i++) {
        if (a[i] == target) return i;
    }
    return -1;
}

int last_index_of(int a[], int n, int target) {
    for (int i = n - 1; i >= 0; i--) {
        if (a[i] == target) return i;
    }
    return -1;
}

int count_value(int a[], int n, int target) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] == target) count++;
    }
    return count;
}

int is_sorted(int a[], int n) {
    for (int i = 0; i + 1 < n; i++) {
        if (a[i] > a[i + 1]) return 0;
    }
    return 1;
}

int main(void) {
    int a[6] = {4, 8, 15, 8, 23, 8};
    printf("index_of 8 = %d\n", index_of(a, 6, 8));
    printf("last_index_of 8 = %d\n", last_index_of(a, 6, 8));
    printf("count_value 8 = %d\n", count_value(a, 6, 8));
    printf("is_sorted = %d\n", is_sorted(a, 6));
    return 0;
}
