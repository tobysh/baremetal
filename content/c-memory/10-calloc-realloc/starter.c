#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;

int *new_zeroed(size_t n) {
    return NULL;
}

int vec_push(IntVec *v, int value) {
    return -1;
}

int vec_shrink(IntVec *v) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    IntVec v = {NULL, 0, 0};
    for (int i = 0; i < 10; i++) {
        vec_push(&v, i * i);
    }
    printf("len = %zu, cap = %zu (expected len 10, cap 16)\n", v.len, v.cap);
    vec_shrink(&v);
    printf("after shrink: len = %zu, cap = %zu (expected 10, 10)\n", v.len, v.cap);
    free(v.data);
    return 0;
}
