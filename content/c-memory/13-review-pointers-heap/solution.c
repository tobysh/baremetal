#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *repeat_str(const char *s, int times) {
    size_t len = strlen(s);
    char *out = malloc(len * (size_t)times + 1);
    if (out == NULL) {
        return NULL;
    }
    char *w = out;
    for (int i = 0; i < times; i++) {
        memcpy(w, s, len);
        w += len;
    }
    *w = '\0';
    return out;
}

int *evens(const int *arr, size_t n, size_t *out_count) {
    int *out = malloc((n == 0 ? 1 : n) * sizeof *out);
    if (out == NULL) {
        *out_count = 0;
        return NULL;
    }
    size_t count = 0;
    for (const int *p = arr; p < arr + n; p++) {
        if (*p % 2 == 0) {
            out[count++] = *p;
        }
    }
    *out_count = count;
    return out;
}

void min_max(const int *arr, size_t n, int *min, int *max) {
    *min = arr[0];
    *max = arr[0];
    for (size_t i = 1; i < n; i++) {
        if (arr[i] < *min) *min = arr[i];
        if (arr[i] > *max) *max = arr[i];
    }
}

int main(void) {
    char *r = repeat_str("ab", 3);
    printf("%s\n", r ? r : "(null)");
    free(r);

    int a[] = {1, 2, 3, 4, 6, 7};
    size_t count;
    int *e = evens(a, 6, &count);
    printf("%zu evens\n", count);
    free(e);

    int lo = 0, hi = 0;
    min_max(a, 6, &lo, &hi);
    printf("min %d max %d\n", lo, hi);
    return 0;
}
