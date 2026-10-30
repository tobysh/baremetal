#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct { unsigned char *base; size_t cap; size_t off; } Arena;

void arena_init(Arena *a, void *buf, size_t cap) {
    a->base = buf;
    a->cap = cap;
    a->off = 0;
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
    if (size == 0 || align == 0 || (align & (align - 1)) != 0) return NULL;
    uintptr_t addr = (uintptr_t)a->base + a->off;
    size_t pad = (align - (addr & (align - 1))) & (align - 1);
    size_t room = a->cap - a->off;
    if (pad > room || size > room - pad) return NULL;
    a->off += pad + size;
    return a->base + a->off - size;
}

size_t arena_used(const Arena *a) {
    return a->off;
}

size_t arena_mark(const Arena *a) {
    return a->off;
}

void arena_rewind(Arena *a, size_t mark) {
    if (mark <= a->off) a->off = mark;
}

void arena_reset(Arena *a) {
    a->off = 0;
}

// Public example: run it and read the offsets.
int main(void) {
    _Alignas(16) unsigned char buf[64];
    Arena a;
    arena_init(&a, buf, sizeof buf);
    char *c = arena_alloc(&a, 3, 1);
    double *d = arena_alloc(&a, sizeof(double), _Alignof(double));
    size_t m = arena_mark(&a);
    int *i = arena_alloc(&a, 4 * sizeof(int), 16);
    printf("c at +%td, d at +%td, i at +%td, used %zu\n",
           c ? (unsigned char *)c - buf : -1, d ? (unsigned char *)d - buf : -1, i ? (unsigned char *)i - buf : -1, arena_used(&a));
    arena_rewind(&a, m);
    printf("after rewind: used %zu (expected %zu)\n", arena_used(&a), m);
    printf("too big: %p (expected (nil))\n", arena_alloc(&a, 1000, 1));
    arena_reset(&a);
    printf("after reset: used %zu\n", arena_used(&a));
    return 0;
}
