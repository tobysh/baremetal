#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *make_range(int n) {
    int *arr = malloc((size_t)n * sizeof *arr);
    if (arr == NULL) {
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        arr[i] = i;
    }
    return arr;
}

char *duplicate(const char *s) {
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, s, len + 1);
    return copy;
}

int main(void) {
    int *r = make_range(5);
    if (r != NULL) {
        for (int i = 0; i < 5; i++) printf("%d ", r[i]);
        printf("\n");
        free(r);
    }
    char *d = duplicate("hello");
    if (d != NULL) {
        printf("%s\n", d);
        free(d);
    }
    return 0;
}
