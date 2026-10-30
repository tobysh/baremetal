#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;

int vec_insert(IntVec *v, size_t i, int value) {
    if (i > v->len) {
        return -1;
    }
    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
        int *tmp = realloc(v->data, new_cap * sizeof *tmp);
        if (tmp == NULL) {
            return -1;
        }
        v->data = tmp;
        v->cap = new_cap;
    }
    memmove(v->data + i + 1, v->data + i, (v->len - i) * sizeof *v->data);
    v->data[i] = value;
    v->len++;
    return 0;
}

int vec_remove(IntVec *v, size_t i, int *out) {
    if (i >= v->len) {
        return -1;
    }
    if (out != NULL) {
        *out = v->data[i];
    }
    memmove(v->data + i, v->data + i + 1, (v->len - i - 1) * sizeof *v->data);
    v->len--;
    if (v->cap > 4 && v->len * 4 <= v->cap) {
        size_t new_cap = v->cap / 2;
        int *tmp = realloc(v->data, new_cap * sizeof *tmp);
        if (tmp != NULL) {
            v->data = tmp;
            v->cap = new_cap;
        }
    }
    return 0;
}

int main(void) {
    IntVec v = {NULL, 0, 0};
    for (int i = 0; i < 5; i++) {
        vec_insert(&v, 0, i);
    }
    int x;
    vec_remove(&v, 1, &x);
    printf("removed %d, len = %zu, cap = %zu\n", x, v.len, v.cap);
    free(v.data);
    return 0;
}
