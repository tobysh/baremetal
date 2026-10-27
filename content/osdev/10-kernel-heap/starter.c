#include <stdint.h>
#include <stddef.h>

typedef struct Block {
    size_t size;            // payload bytes, not counting this header
    int free;
    struct Block *next;     // next block in address order
} __attribute__((aligned(16))) Block;

static Block *head;

void heap_init(void *start, size_t size) {
}

void *kmalloc(size_t n) {
    return NULL;
}

void kfree(void *p) {
}

void *kcalloc(size_t count, size_t size) {
    return NULL;
}

size_t heap_free_bytes(void) {
    return 0;
}
