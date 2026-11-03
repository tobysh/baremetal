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

_Alignas(16) static unsigned char small_pool[8][16], large_pool[8][64];
static unsigned char small_used[8], large_used[8];

void classes_init(void *buf, size_t n) {
    memset(small_used, 0, 8);
    memset(large_used, 0, 8);
    heap_init(buf, n);
}

void *classes_alloc(size_t n) {
    if (!n) return NULL;
    if (n <= 64) {
        int large = n > 16;
        unsigned char *used = large ? large_used : small_used;
        for (int i = 0; i < 8; i++) {
            if (!used[i]) {
                used[i] = 1;
                return large ? (void *)large_pool[i] : (void *)small_pool[i];
            }
        }
    }
    return heap_alloc(n);
}

int classes_free(void *p) {
    if (!p) return 0;
    uintptr_t x = (uintptr_t)p;
    for (int large = 0; large < 2; large++) {
        unsigned char *lo = large ? &large_pool[0][0] : &small_pool[0][0];
        size_t stride = large ? 64 : 16;
        uintptr_t start = (uintptr_t)lo;
        if (x >= start && x < start + 8 * stride) {
            size_t off = x - start;
            unsigned char *used = large ? large_used : small_used;
            if (off % stride || !used[off / stride]) return -1;
            used[off / stride] = 0;
            return 0;
        }
    }
    return heap_free(p);
}

int main(void) {
    _Alignas(16) unsigned char buf[1024];
    classes_init(buf, sizeof buf);
    void *p = classes_alloc(12);
    printf("allocated: %d\n", p != NULL);
    printf("freed: %d\n", classes_free(p));
    return 0;
}
