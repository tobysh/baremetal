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
    p->base = buf; p->slot = 0; p->count = 0; p->free_head = NULL; p->nfree = 0;
    if (objsize == 0 || objsize > SIZE_MAX - 8) return 0;
    size_t slot = objsize < 8 ? 8 : (objsize + 7) & ~(size_t)7;
    size_t count = cap / slot;
    if (count == 0) return 0;
    p->slot = slot; p->count = count; p->nfree = count;
    for (size_t i = 0; i < count; i++) {
        set_next(p->base + i * slot, i + 1 < count ? p->base + (i + 1) * slot : NULL);
    }
    p->free_head = p->base;
    return (int)count;
}

void *pool_alloc(Pool *p) {
    void *s = p->free_head;
    if (!s) return NULL;
    p->free_head = next_of(s);
    p->nfree--;
    return s;
}

int pool_free(Pool *p, void *ptr) {
    if (!ptr || p->count == 0) return -1;
    uintptr_t a = (uintptr_t)ptr, lo = (uintptr_t)p->base;
    if (a < lo || a >= lo + p->count * p->slot || (a - lo) % p->slot != 0) return -1;
    for (void *f = p->free_head; f; f = next_of(f)) if (f == ptr) return -1;
    set_next(ptr, p->free_head);
    p->free_head = ptr;
    p->nfree++;
    return 0;
}

size_t pool_available(const Pool *p) { return p->nfree; }

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
