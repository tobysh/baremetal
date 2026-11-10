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

/* Removes blocks[i], shifting the later descriptors down. */
static void remove_block(unsigned i) {
    for (unsigned j = i; j + 1 < count; j++)
        blocks[j] = blocks[j + 1];
    count--;
}

void heap_init(void) {
    blocks[0] = (Block){ 0, ARENA_SIZE, 0 };
    count = 1;
}

void *heap_alloc(size_t n) {
    if (n == 0 || n > ARENA_SIZE)
        return NULL;                    /* check before rounding: no overflow */
    n = (n + 7) & ~(size_t)7;

    for (unsigned i = 0; i < count; i++) {
        if (blocks[i].busy || blocks[i].len < n)
            continue;
        if (blocks[i].len > n) {
            /* Split: the tail becomes a new free block at i + 1. */
            if (count == MAX_BLOCKS)
                return NULL;
            for (unsigned j = count; j > i + 1; j--)
                blocks[j] = blocks[j - 1];
            blocks[i + 1] = (Block){ blocks[i].off + n, blocks[i].len - n, 0 };
            count++;
            blocks[i].len = n;
        }
        blocks[i].busy = 1;
        return arena + blocks[i].off;
    }
    return NULL;                        /* no free block is large enough */
}

int heap_free(void *p) {
    for (unsigned i = 0; i < count; i++) {
        if (p != arena + blocks[i].off)
            continue;
        if (!blocks[i].busy)
            return -1;                  /* double free */
        blocks[i].busy = 0;

        /* Merge with a free predecessor: it absorbs this block. */
        if (i > 0 && !blocks[i - 1].busy) {
            blocks[i - 1].len += blocks[i].len;
            remove_block(i);
            i--;
        }
        /* Merge with a free successor: this block absorbs it. */
        if (i + 1 < count && !blocks[i + 1].busy) {
            blocks[i].len += blocks[i + 1].len;
            remove_block(i + 1);
        }
        return 0;
    }
    return -1;                          /* not the start of any block */
}
