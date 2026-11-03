#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define HDR 16
#define MINP 16

typedef struct Block { size_t size; size_t is_free; } Block;   // 16 bytes, size = payload bytes
typedef struct {
    size_t used_bytes, free_bytes, used_blocks, free_blocks, largest_free;
    unsigned long search_steps;   // blocks examined by searches since heap_init
    unsigned long failed_allocs;  // heap_alloc(n > 0) calls that returned NULL
} HeapStats;
typedef enum { FIRST_FIT, BEST_FIT } Policy;

static unsigned char *heap_lo;
static size_t heap_len;
static Policy policy;
static unsigned long steps, failed;

static Block *first_block(void) { return heap_len ? (Block *)heap_lo : NULL; }
static Block *next_block(Block *b) {
    unsigned char *n = (unsigned char *)b + HDR + b->size;
    return n < heap_lo + heap_len ? (Block *)n : NULL;
}

void heap_set_policy(Policy p) { policy = p; }

void heap_init(void *buf, size_t size) {
    heap_lo = buf;
    policy = FIRST_FIT; steps = 0; failed = 0;
    heap_len = size & ~(size_t)15;
    if (heap_len < HDR + MINP) { heap_len = 0; return; }
    Block *b = (Block *)heap_lo;
    b->size = heap_len - HDR;
    b->is_free = 1;
}

static Block *find_block(size_t need) {
    Block *best = NULL;
    for (Block *b = first_block(); b; b = next_block(b)) {
        steps++;
        if (!b->is_free || b->size < need) continue;
        if (policy == FIRST_FIT) return b;
        if (!best || b->size < best->size) best = b;
    }
    return best;
}

void *heap_alloc(size_t n) {
    if (n == 0) return NULL;
    if (n > heap_len) { failed++; return NULL; }
    size_t need = (n + 15) & ~(size_t)15;
    Block *b = find_block(need);
    if (!b) { failed++; return NULL; }
    if (b->size >= need + HDR + MINP) {
        Block *rest = (Block *)((unsigned char *)b + HDR + need);
        rest->size = b->size - need - HDR;
        rest->is_free = 1;
        b->size = need;
    }
    b->is_free = 0;
    return (unsigned char *)b + HDR;
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
    s->search_steps = steps; s->failed_allocs = failed;
    for (Block *b = first_block(); b; b = next_block(b)) {
        if (b->is_free) {
            s->free_blocks++; s->free_bytes += b->size;
            if (b->size > s->largest_free) s->largest_free = b->size;
        } else { s->used_blocks++; s->used_bytes += b->size; }
    }
}

size_t heap_map(char *out, size_t cap) {
    size_t n = 0;
    for (Block *b = first_block(); b; b = next_block(b)) {
        if (cap && n < cap - 1) out[n] = 'H';
        n++;
        for (size_t i = 0; i < b->size / 16; i++) {
            if (cap && n < cap - 1) out[n] = b->is_free ? '.' : 'U';
            n++;
        }
    }
    if (cap) out[n < cap ? n : cap - 1] = 0;
    return n;
}

int main(void) {
    _Alignas(16) unsigned char buf[128];
    char map[32];
    heap_init(buf, sizeof buf);
    heap_alloc(16);
    heap_map(map, sizeof map);
    puts(map);
    return 0;
}
