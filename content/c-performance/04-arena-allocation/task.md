Implement `arena_alloc`, `arena_reset` and `arena_destroy` following the policy in the lesson.

- `arena_alloc(a, n, align)` returns a pointer aligned to `align` (a power of two, 1 to 64) with `n` usable bytes, never overlapping another live block. `n` is at least 1.
- The current chunk is the last one. Round up its offset to `align`; if `offset + n` exceeds its size, request a new chunk of `n > CHUNK ? n : CHUNK` bytes and place the block at its start.
- On `sys_alloc` failure or when `nchunks == MAXCHUNKS`, return `NULL` and change nothing.
- `arena_reset` frees all chunks except the first and rewinds to its start; with no chunk it does nothing.
- `arena_destroy` frees every chunk. Afterwards `sys_live` must be 0 and `sys_frees == sys_allocs`.
- Call `sys_alloc` only when a chunk is really needed. The tests count the calls exactly.

Do not edit `sys_alloc`, `sys_free` or the counters.
