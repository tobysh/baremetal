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

int vec_insert(IntVec *v, size_t i, int x) {
    if (i > v->len) return -1;
    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
        int *bigger = realloc(v->data, new_cap * sizeof *bigger);
        if (bigger == NULL) return -1;
        v->data = bigger;
        v->cap = new_cap;
    }
    memmove(v->data + i + 1, v->data + i, (v->len - i) * sizeof *v->data);
    v->data[i] = x;
    v->len++;
    return 0;
}

int vec_remove_at(IntVec *v, size_t i, int *out) {
    if (i >= v->len) return -1;
    if (out) *out = v->data[i];
    memmove(v->data + i, v->data + i + 1, (v->len - i - 1) * sizeof *v->data);
    v->len--;
    if (v->cap > 4 && v->len <= v->cap / 4) {
        int *smaller = realloc(v->data, (v->cap / 2) * sizeof *smaller);
        if (smaller != NULL) {
            v->data = smaller;
            v->cap /= 2;
        }
    }
    return 0;
}

// Optional: try them with Run.
int main(void) {
    IntVec v;
    vec_init(&v);
    for (int i = 0; i < 5; i++) vec_push(&v, i);
    vec_insert(&v, 2, 99);
    for (size_t i = 0; i < v.len; i++) printf("%d ", v.data[i]);
    printf("(expected 0 1 99 2 3 4)\n");
    vec_free(&v);
    return 0;
}
