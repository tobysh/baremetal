#include <stdio.h>
#include <string.h>

int insertion_sort(int *a, int n) {
    return -1;
}

void insertion_sort_strs(const char **a, int n) {
}

// Optional: try it with Run.
int main(void) {
    int a[] = {5, 2, 4, 6, 1, 3};
    int shifts = insertion_sort(a, 6);
    for (int i = 0; i < 6; i++) printf("%d ", a[i]);
    printf("(%d shifts, expected 9)\n", shifts);
    return 0;
}
