Loop with `for (Block *b = first_block(); b; b = next_block(b))`. Skip blocks that are in use or too small.
===
Splitting: the new header goes at `(unsigned char *)b + HDR + need`. Its size is `b->size - need - HDR`. Then shrink `b->size` to `need`. Only do this when `b->size >= need + HDR + MINP`.
===
For `heap_free` walk with a `prev` pointer that follows `b`. When `b`'s payload address equals `p`, check `is_free` (double free), mark it free, then merge with `next_block(b)` if free.
===
After merging forward, merge backward: if `prev && prev->is_free`, then `prev->size += HDR + b->size`. Do the forward merge first, otherwise `b->size` is stale.
===
Rounding: `need = (n + 15) & ~15`. The `n > heap_len` test must come first so the addition cannot overflow.
