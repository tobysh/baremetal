**Challenge: `heap_realloc`.** Add to the finished heap

```c
void *heap_realloc(void *p, size_t n);
```

- `p == NULL` behaves like `heap_alloc(n)`.
- `p` that is not a used payload start returns `NULL` and changes nothing.
- `n == 0` frees `p` and returns `NULL`.
- The new size is `need = n` rounded up to 16. **Shrink** (`need <= current size`): stay in place, split off the tail when at least `HDR + 16` bytes remain, and merge that tail with a free block that follows. Return `p`.
- **Grow in place**: if the next block is free and the two together (plus the absorbed header) reach `need`, absorb it, split any large leftover, and return `p` unchanged.
- Otherwise allocate a new block (first fit, **before** freeing the old one), copy the old contents, free the old block (coalescing) and return the new pointer. If no block is available, return `NULL` and leave `p` valid and untouched.
- Contents up to `min(old, new)` bytes must survive in every case.

The tests compare with a segment model, count how often a block moved, and run seeded random sequences that mix realloc with alloc and free. The plain lesson solution does not define `heap_realloc`, so it cannot pass.
