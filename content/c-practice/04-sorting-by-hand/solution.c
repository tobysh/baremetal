#include <stdio.h>
#include <stdlib.h>

void insertion_sort(int *a, int n) {
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

int merge_sorted(const int *a, int na, const int *b, int nb, int *out) {
    int i = 0, j = 0, k = 0;
    while (i < na && j < nb) {
        if (b[j] < a[i]) {
            out[k++] = b[j++];
        } else {
            out[k++] = a[i++];
        }
    }
    while (i < na) {
        out[k++] = a[i++];
    }
    while (j < nb) {
        out[k++] = b[j++];
    }
    return na + nb;
}

static void sort_rec(int *a, int n, int *tmp) {
    if (n < 2) {
        return;
    }
    int mid = n / 2;
    sort_rec(a, mid, tmp);
    sort_rec(a + mid, n - mid, tmp);
    merge_sorted(a, mid, a + mid, n - mid, tmp);
    for (int i = 0; i < n; i++) {
        a[i] = tmp[i];
    }
}

int merge_sort(int *a, int n) {
    if (n < 2) {
        return 0;
    }
    int *tmp = malloc((size_t)n * sizeof(int));
    if (tmp == NULL) {
        return -1;
    }
    sort_rec(a, n, tmp);
    free(tmp);
    return 0;
}

int main(void) {
    int a[] = {5, 2, 9, 1};
    insertion_sort(a, 4);
    for (int i = 0; i < 4; i++) printf("%d ", a[i]);
    printf("\n");
    return 0;
}
