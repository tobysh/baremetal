#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int key;
    int order;
} Rec;

void merge(const int *a, size_t na, const int *b, size_t nb, int *out) {
    size_t i = 0, j = 0, k = 0;
    while (i < na && j < nb) {
        if (b[j] < a[i]) out[k++] = b[j++];
        else out[k++] = a[i++];
    }
    while (i < na) out[k++] = a[i++];
    while (j < nb) out[k++] = b[j++];
}

static void sort_range(int *a, int *tmp, size_t n) {
    if (n < 2) return;
    size_t mid = n / 2;
    sort_range(a, tmp, mid);
    sort_range(a + mid, tmp, n - mid);
    merge(a, mid, a + mid, n - mid, tmp);
    memcpy(a, tmp, n * sizeof *a);
}

int merge_sort(int *a, size_t n) {
    if (n < 2) return 0;
    int *tmp = malloc(n * sizeof *tmp);
    if (tmp == NULL) return -1;
    sort_range(a, tmp, n);
    free(tmp);
    return 0;
}

static void sort_recs(Rec *a, Rec *tmp, size_t n) {
    if (n < 2) return;
    size_t mid = n / 2;
    sort_recs(a, tmp, mid);
    sort_recs(a + mid, tmp, n - mid);
    size_t i = 0, j = mid, k = 0;
    while (i < mid && j < n) {
        if (a[j].key < a[i].key) tmp[k++] = a[j++];
        else tmp[k++] = a[i++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < n) tmp[k++] = a[j++];
    memcpy(a, tmp, n * sizeof *a);
}

int merge_sort_recs(Rec *a, size_t n) {
    if (n < 2) return 0;
    Rec *tmp = malloc(n * sizeof *tmp);
    if (tmp == NULL) return -1;
    sort_recs(a, tmp, n);
    free(tmp);
    return 0;
}

int main(void) {
    int a[] = {1, 4, 9};
    int b[] = {2, 3, 10, 11};
    int out[7];
    merge(a, 3, b, 4, out);
    for (int i = 0; i < 7; i++) printf("%d ", out[i]);
    printf("\n");
    return 0;
}
