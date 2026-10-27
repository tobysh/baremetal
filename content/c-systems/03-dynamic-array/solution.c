#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;

void vec_init(IntVec *v) {
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

int vec_push(IntVec *v, int x) {
    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
        int *bigger = realloc(v->data, new_cap * sizeof *bigger);
        if (bigger == NULL) {
            return -1;
        }
        v->data = bigger;
        v->cap = new_cap;
    }
    v->data[v->len] = x;
    v->len++;
    return 0;
}

int vec_pop(IntVec *v, int *out) {
    if (v->len == 0) {
        return -1;
    }
    v->len--;
    *out = v->data[v->len];
    return 0;
}

void vec_free(IntVec *v) {
    free(v->data);
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

int main(void) {
    IntVec v;
    vec_init(&v);
    for (int i = 0; i < 10; i++) vec_push(&v, i * i);
    printf("len = %zu, cap = %zu\n", v.len, v.cap);
    vec_free(&v);
    return 0;
}
