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
        int *p = realloc(v->data, new_cap * sizeof *p);
        if (p == NULL) {
            return -1;
        }
        v->data = p;
        v->cap = new_cap;
    }
    v->data[v->len++] = x;
    return 0;
}

void vec_free(IntVec *v) {
    free(v->data);
    vec_init(v);
}

void vec_map(IntVec *v, int (*f)(int)) {
}

size_t vec_filter(IntVec *v, int (*keep)(int)) {
    return 0;
}

int vec_reduce(const IntVec *v, int init, int (*f)(int acc, int x)) {
    return 0;
}

static int demo_twice(int x) { return x * 2; }
static int demo_add(int acc, int x) { return acc + x; }

// Optional: try them with Run.
int main(void) {
    IntVec v;
    vec_init(&v);
    for (int i = 1; i <= 5; i++) vec_push(&v, i);
    vec_map(&v, demo_twice);
    printf("sum of doubled = %d (expected 30)\n", vec_reduce(&v, 0, demo_add));
    vec_free(&v);
    return 0;
}
