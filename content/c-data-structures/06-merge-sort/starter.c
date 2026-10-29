#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int key;
    int order;
} Rec;

void merge(const int *a, size_t na, const int *b, size_t nb, int *out) {
}

int merge_sort(int *a, size_t n) {
    return -1;
}

int merge_sort_recs(Rec *a, size_t n) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    int a[] = {1, 4, 9};
    int b[] = {2, 3, 10, 11};
    int out[7];
    merge(a, 3, b, 4, out);
    for (int i = 0; i < 7; i++) printf("%d ", out[i]);
    printf("\n");
    return 0;
}
