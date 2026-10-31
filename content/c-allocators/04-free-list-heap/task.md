Complete `heap_alloc` and `heap_free` in the starter. The block layout, walkers, `heap_init` and `heap_stats` are given.

Rules (the tests model them exactly):

- Payload sizes are multiples of 16, minimum 16; header is 16 bytes. Returned pointers are 16-aligned.
- `heap_alloc(n)`: `NULL` for `n == 0` or `n` bigger than the heap. First fit in address order. Split when `block->size >= need + 32`, otherwise use the whole block. A failed allocation changes nothing.
- `heap_free(p)`: `NULL` returns 0. A valid used payload is freed and merged with the free block after it and the free block before it (so the heap never holds two adjacent free blocks); return 0. Anything else (interior pointer, foreign pointer, already free) returns -1 and changes nothing.

Hidden tests replay seeded random allocate/free sequences (with bad frees) against an independent segment-array model, check alignment, fill every live block with its own byte pattern to catch overlap, then free everything in a shuffled order and demand a single free block of `len - 16` bytes.
