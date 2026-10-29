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
    h->a = NULL;
    h->len = 0;
    h->cap = 0;
    if (cap == 0) return -1;
    h->a = malloc(cap * sizeof *h->a);
    if (h->a == NULL) return -1;
    h->cap = cap;
    return 0;
}

int heap_push(Heap *h, int x) {
    if (h->len == h->cap) return -1;
    size_t i = h->len++;
    h->a[i] = x;
    while (i > 0 && h->a[(i - 1) / 2] > h->a[i]) {
        swap(&h->a[(i - 1) / 2], &h->a[i]);
        i = (i - 1) / 2;
    }
    return 0;
}

int heap_pop(Heap *h, int *out) {
    if (h->len == 0) return -1;
    *out = h->a[0];
    h->len--;
    if (h->len > 0) {
        h->a[0] = h->a[h->len];
        size_t i = 0;
        for (;;) {
            size_t l = 2 * i + 1, r = l + 1, smallest = i;
            if (l < h->len && h->a[l] < h->a[smallest]) smallest = l;
            if (r < h->len && h->a[r] < h->a[smallest]) smallest = r;
            if (smallest == i) break;
            swap(&h->a[i], &h->a[smallest]);
            i = smallest;
        }
    }
    return 0;
}

int heap_peek(const Heap *h, int *out) {
    if (h->len == 0) return -1;
    *out = h->a[0];
    return 0;
}

void heap_free(Heap *h) {
    free(h->a);
    h->a = NULL;
    h->len = 0;
    h->cap = 0;
}

int heap_sort(int *a, size_t n) {
    if (n < 2) return 0;
    Heap h;
    if (heap_init(&h, n) != 0) return -1;
    for (size_t i = 0; i < n; i++) heap_push(&h, a[i]);
    for (size_t i = 0; i < n; i++) heap_pop(&h, &a[i]);
    heap_free(&h);
    return 0;
}

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
