#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;

int *new_zeroed(size_t n) {
    return calloc(n, sizeof(int));
}

int vec_push(IntVec *v, int value) {
    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
        int *tmp = realloc(v->data, new_cap * sizeof *tmp);
        if (tmp == NULL) {
            return -1;
        }
        v->data = tmp;
        v->cap = new_cap;
    }
    v->data[v->len++] = value;
    return 0;
}

int vec_shrink(IntVec *v) {
    if (v->len == 0) {
        free(v->data);
        v->data = NULL;
        v->cap = 0;
        return 0;
    }
    int *tmp = realloc(v->data, v->len * sizeof *tmp);
    if (tmp != NULL) {
        v->data = tmp;
        v->cap = v->len;
    }
    return 0;
}

int main(void) {
    IntVec v = {NULL, 0, 0};
    for (int i = 0; i < 10; i++) {
        vec_push(&v, i * i);
    }
    printf("len = %zu, cap = %zu\n", v.len, v.cap);
    vec_shrink(&v);
    printf("after shrink: len = %zu, cap = %zu\n", v.len, v.cap);
    free(v.data);
    return 0;
}
