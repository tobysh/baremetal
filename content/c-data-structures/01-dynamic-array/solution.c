#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int reserve_one(IntVec *v) {
    if (v->len < v->cap) return 0;
    size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
    int *bigger = realloc(v->data, new_cap * sizeof *bigger);
    if (bigger == NULL) return -1;
    v->data = bigger;
    v->cap = new_cap;
    return 0;
}

int vec_push(IntVec *v, int x) {
    if (reserve_one(v) != 0) return -1;
    v->data[v->len++] = x;
    return 0;
}

int vec_get(const IntVec *v, size_t i, int *out) {
    if (i >= v->len) return -1;
    *out = v->data[i];
    return 0;
}

int vec_pop(IntVec *v, int *out) {
    if (v->len == 0) return -1;
    *out = v->data[--v->len];
    return 0;
}

int vec_insert(IntVec *v, size_t i, int x) {
    if (i > v->len) return -1;
    if (reserve_one(v) != 0) return -1;
    memmove(&v->data[i + 1], &v->data[i], (v->len - i) * sizeof v->data[0]);
    v->data[i] = x;
    v->len++;
    return 0;
}

void vec_free(IntVec *v) {
    free(v->data);
    vec_init(v);
}

int main(void) {
    IntVec v;
    vec_init(&v);
    for (int i = 0; i < 10; i++) vec_push(&v, i * i);
    printf("len %zu, cap %zu\n", v.len, v.cap);
    vec_free(&v);
    return 0;
}
