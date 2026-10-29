#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *a;
    size_t len;
    size_t cap;
} Heap;

static void swap(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}

int heap_init(Heap *h, size_t cap) {
    return -1;
}

int heap_push(Heap *h, int x) {
    return -1;
}

int heap_pop(Heap *h, int *out) {
    return -1;
}

int heap_peek(const Heap *h, int *out) {
    return -1;
}

void heap_free(Heap *h) {
}

int heap_sort(int *a, size_t n) {
    return -1;
}

// Optional: try it with Run.
int main(void) {
    Heap h;
    heap_init(&h, 8);
    int xs[] = {5, 3, 8, 1, 9};
    for (int i = 0; i < 5; i++) heap_push(&h, xs[i]);
    int v;
    heap_pop(&h, &v);
    printf("smallest %d (expected 1)\n", v);
    heap_free(&h);
    return 0;
}
