#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    unsigned char *base;
    size_t slot;
    size_t count;
    void *free_head;
    size_t nfree;
} Pool;

static void *next_of(void *slot) { void *n; memcpy(&n, slot, sizeof n); return n; }
static void set_next(void *slot, void *n) { memcpy(slot, &n, sizeof n); }

int pool_init(Pool *p, void *buf, size_t cap, size_t objsize) {
    return 0;
}

void *pool_alloc(Pool *p) {
    return NULL;
}

int pool_free(Pool *p, void *ptr) {
    return 0;
}

size_t pool_available(const Pool *p) { return 0; }

// Public example.
int main(void) {
    _Alignas(8) unsigned char buf[100];
    Pool p;
    int n = pool_init(&p, buf, sizeof buf, 20);   // slots of 24 bytes -> 4 fit
    printf("slots: %d (expected 4)\n", n);
    void *a = pool_alloc(&p), *b = pool_alloc(&p);
    printf("a=+%td b=+%td (expected +0 and +24)\n", (unsigned char *)a - buf, (unsigned char *)b - buf);
    pool_free(&p, a);
    printf("reused: %s\n", pool_alloc(&p) == a ? "yes" : "no");
    printf("double free: %d (expected -1)\n", pool_free(&p, b) + pool_free(&p, b));
    printf("available: %zu\n", pool_available(&p));
    return 0;
}
