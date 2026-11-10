The 256-byte `arena` and the descriptor array `blocks` (32 entries, `count` in use, kept in address order) are in the starter. Implement:

- `void heap_init(void)` resets the heap to a single free block covering the whole arena.
- `void *heap_alloc(size_t n)` returns `NULL` for `n == 0` or `n > 256`. Otherwise it rounds `n` up to a multiple of 8 and takes the **first** free block (lowest address) that is large enough. A larger block is split: the front part becomes the busy allocation and the rest stays free. It returns `arena + offset` of the allocation, or `NULL` when no free block fits or a split would need a 33rd descriptor. On failure nothing changes.
- `int heap_free(void *p)` returns 0 when `p` is the start of a busy block: the block becomes free and is merged with a free block directly before it and a free block directly after it. It returns -1 and changes nothing for `NULL`, a pointer outside the arena, a pointer into the middle of a block, or a block that is already free.

The tests check offsets of returned pointers, reuse of holes, merging in both directions, that non-adjacent free blocks are not merged, and invalid frees.
