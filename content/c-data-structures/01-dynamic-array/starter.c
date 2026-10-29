#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;

void vec_init(IntVec *v) {
}

int vec_push(IntVec *v, int x) {
    return -1;
}

int vec_get(const IntVec *v, size_t i, int *out) {
    return -1;
}

int vec_pop(IntVec *v, int *out) {
    return -1;
}

int vec_insert(IntVec *v, size_t i, int x) {
    return -1;
}

void vec_free(IntVec *v) {
}

// Optional: try it with Run.
int main(void) {
    IntVec v;
    vec_init(&v);
    for (int i = 0; i < 10; i++) vec_push(&v, i * i);
    printf("len %zu, cap %zu\n", v.len, v.cap);
    vec_free(&v);
    return 0;
}
