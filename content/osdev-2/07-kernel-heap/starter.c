#include <stddef.h>
#include <stdint.h>

#define ARENA_SIZE 256
#define MAX_BLOCKS 32

static unsigned char arena[ARENA_SIZE] __attribute__((aligned(8)));

/* One descriptor per block, in address order. The blocks tile the arena:
   blocks[i + 1].off == blocks[i].off + blocks[i].len. */
typedef struct {
    size_t off;   /* offset of the block in the arena */
    size_t len;   /* length in bytes, a multiple of 8 */
    int busy;     /* 1 if allocated, 0 if free */
} Block;

static Block blocks[MAX_BLOCKS];
static unsigned count;   /* blocks[0 .. count-1] are valid */

void heap_init(void) {
    /* TODO: one free block covering the whole arena */
}

void *heap_alloc(size_t n) {
    /* TODO: validate n, round to 8, first fit, split if larger */
    (void)n;
    return NULL;
}

int heap_free(void *p) {
    /* TODO: find the busy block starting at p, free it, merge neighbours */
    (void)p;
    return -1;
}
