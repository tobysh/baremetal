The `Block` header is defined in the starter. Implement:

- `void heap_init(void *start, size_t size)` aligns `start` up to a multiple of 16, shrinks `size` to match (and to a multiple of 16), and sets up one free block covering all of it. If there is no room for a header plus 16 bytes, the heap is empty.
- `void *kmalloc(size_t n)` as described: 0 gives `NULL`, sizes are rounded up to 16, first fit, split when `size >= wanted + sizeof(Block) + 16`.
- `void kfree(void *p)` marks the block free and merges every adjacent pair of free blocks. `NULL` is ignored.
- `void *kcalloc(size_t count, size_t size)` returns zeroed memory, or `NULL` if the multiplication would overflow or memory is gone.
- `size_t heap_free_bytes(void)` returns the total payload size of all free blocks.
