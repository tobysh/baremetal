#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define HDR 16
#define MINP 16

typedef struct Block { size_t size; size_t is_free; } Block;   // 16 bytes, size = payload bytes
typedef struct { size_t used_bytes, free_bytes, used_blocks, free_blocks, largest_free; } HeapStats;

static unsigned char *heap_lo;
static size_t heap_len;

static Block *first_block(void) { return heap_len ? (Block *)heap_lo : NULL; }
static Block *next_block(Block *b) {
    unsigned char *n = (unsigned char *)b + HDR + b->size;
    return n < heap_lo + heap_len ? (Block *)n : NULL;
}

void heap_init(void *buf, size_t size) {
    heap_lo = buf;
    heap_len = size & ~(size_t)15;
    if (heap_len < HDR + MINP) { heap_len = 0; return; }
    Block *b = (Block *)heap_lo;
    b->size = heap_len - HDR;
    b->is_free = 1;
}

void *heap_alloc(size_t n) {
    if (n == 0 || n > heap_len) return NULL;
    size_t need = (n + 15) & ~(size_t)15;
    for (Block *b = first_block(); b; b = next_block(b)) {
        if (!b->is_free || b->size < need) continue;
        if (b->size >= need + HDR + MINP) {
            Block *rest = (Block *)((unsigned char *)b + HDR + need);
            rest->size = b->size - need - HDR;
            rest->is_free = 1;
            b->size = need;
        }
        b->is_free = 0;
        return (unsigned char *)b + HDR;
    }
    return NULL;
}

int heap_free(void *p) {
    if (!p) return 0;
    Block *prev = NULL;
    for (Block *b = first_block(); b; prev = b, b = next_block(b)) {
        if ((unsigned char *)b + HDR != (unsigned char *)p) continue;
        if (b->is_free) return -1;
        b->is_free = 1;
        Block *n = next_block(b);
        if (n && n->is_free) b->size += HDR + n->size;
        if (prev && prev->is_free) { prev->size += HDR + b->size; }
        return 0;
    }
    return -1;
}

void heap_stats(HeapStats *s) {
    memset(s, 0, sizeof *s);
    for (Block *b = first_block(); b; b = next_block(b)) {
        if (b->is_free) {
            s->free_blocks++; s->free_bytes += b->size;
            if (b->size > s->largest_free) s->largest_free = b->size;
        } else { s->used_blocks++; s->used_bytes += b->size; }
    }
}

// Public example: watch the stats while blocks are freed in different orders.
int main(void) {
    _Alignas(16) static unsigned char buf[512];
    HeapStats st;
    heap_init(buf, sizeof buf);
    void *a = heap_alloc(40), *b = heap_alloc(40), *c = heap_alloc(40);
    heap_stats(&st);
    printf("after 3 allocs: used=%zu free=%zu free_blocks=%zu\n", st.used_bytes, st.free_bytes, st.free_blocks);
    heap_free(a); heap_free(c);
    heap_stats(&st);
    printf("freed a and c: free_blocks=%zu\n", st.free_blocks);
    heap_free(b);
    heap_stats(&st);
    printf("freed b: free_blocks=%zu largest=%zu (expected 1 and 496)\n", st.free_blocks, st.largest_free);
    return 0;
}
