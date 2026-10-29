#include <stdio.h>
#include <string.h>

int insertion_sort(int *a, int n) {
    int shifts = 0;
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
            shifts++;
        }
        a[j + 1] = key;
    }
    return shifts;
}

void insertion_sort_strs(const char **a, int n) {
    for (int i = 1; i < n; i++) {
        const char *key = a[i];
        int j = i - 1;
        while (j >= 0 && strcmp(a[j], key) > 0) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

int main(void) {
    int a[] = {5, 2, 4, 6, 1, 3};
    int shifts = insertion_sort(a, 6);
    for (int i = 0; i < 6; i++) printf("%d ", a[i]);
    printf("(%d shifts, expected 9)\n", shifts);
    return 0;
}
