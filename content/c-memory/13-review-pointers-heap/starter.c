#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *repeat_str(const char *s, int times) {
    return NULL;
}

int *evens(const int *arr, size_t n, size_t *out_count) {
    *out_count = 0;
    return NULL;
}

void min_max(const int *arr, size_t n, int *min, int *max) {
}

// Optional: try them with Run.
int main(void) {
    char *r = repeat_str("ab", 3);
    printf("%s (expected ababab)\n", r ? r : "(null)");
    free(r);

    int a[] = {1, 2, 3, 4, 6, 7};
    size_t count;
    int *e = evens(a, 6, &count);
    printf("%zu evens (expected 3)\n", count);
    free(e);

    int lo = 0, hi = 0;
    min_max(a, 6, &lo, &hi);
    printf("min %d max %d (expected 1 and 7)\n", lo, hi);
    return 0;
}
