#include <stdint.h>
#include <stddef.h>

typedef struct Block {
    size_t size;            // payload bytes, not counting this header
    int free;
    struct Block *next;     // next block in address order
} __attribute__((aligned(16))) Block;

static Block *head;

void heap_init(void *start, size_t size) {
    uintptr_t s = (uintptr_t)start;
    uintptr_t aligned = (s + 15) & ~(uintptr_t)15;
    head = NULL;
    if (size < aligned - s) return;
    size -= aligned - s;
    size &= ~(size_t)15;
    if (size < sizeof(Block) + 16) return;
    head = (Block *)aligned;
    head->size = size - sizeof(Block);
    head->free = 1;
    head->next = NULL;
}

void *kmalloc(size_t n) {
    if (n == 0 || n > (size_t)-1 - 16) return NULL;
    size_t want = (n + 15) & ~(size_t)15;
    for (Block *b = head; b; b = b->next) {
        if (!b->free || b->size < want) continue;
        if (b->size >= want + sizeof(Block) + 16) {
            Block *rest = (Block *)((uint8_t *)(b + 1) + want);
            rest->size = b->size - want - sizeof(Block);
            rest->free = 1;
            rest->next = b->next;
            b->next = rest;
            b->size = want;
        }
        b->free = 0;
        return b + 1;
    }
    return NULL;
}

void kfree(void *p) {
    if (!p) return;
    ((Block *)p - 1)->free = 1;
    for (Block *b = head; b && b->next;) {
        if (b->free && b->next->free) {
            b->size += sizeof(Block) + b->next->size;
            b->next = b->next->next;
        } else {
            b = b->next;
        }
    }
}

void *kcalloc(size_t count, size_t size) {
    if (size && count > (size_t)-1 / size) return NULL;
    size_t total = count * size;
    uint8_t *p = kmalloc(total);
    if (p) for (size_t i = 0; i < total; i++) p[i] = 0;
    return p;
}

size_t heap_free_bytes(void) {
    size_t total = 0;
    for (Block *b = head; b; b = b->next)
        if (b->free) total += b->size;
    return total;
}
