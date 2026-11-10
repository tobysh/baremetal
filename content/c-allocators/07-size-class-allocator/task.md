Write `classes_init`, `classes_alloc` and `classes_free`. The heap (`heap_init`, `heap_alloc`, `heap_free`, `heap_stats`) and the pool storage (`small_pool`, `large_pool`, `small_used`, `large_used`) are supplied; do not edit them.

**`void classes_init(void *buf, size_t n)`** clears every `used` flag of both pools and calls `heap_init(buf, n)`. Calling it again starts from scratch.

**`void *classes_alloc(size_t n)`**
- `n == 0` returns `NULL`.
- `1 <= n <= 16`: use the **lowest-numbered** free slot of `small_pool`, mark it used and return its address.
- `17 <= n <= 64`: the same with `large_pool` (64-byte slots).
- If the preferred pool has no free slot, **fall back** to `heap_alloc(n)`. A full small pool does not borrow large slots, and the other way round.
- `n > 64`: `heap_alloc(n)`.
- Return `NULL` only when the heap cannot serve the request either. Every returned pointer is 16-byte aligned and distinct from every live allocation; contents of live allocations are never altered.

**`int classes_free(void *p)`**
- `NULL` returns 0 and changes nothing.
- A pointer inside a pool's storage must be the exact start of a slot that is currently used. Then clear its flag and return 0. An interior pointer or an already-free slot returns -1 and changes no flag.
- Any other pointer is passed to `heap_free(p)`, and its result (0 or -1) is returned.
- A freed slot is reused by the next request of that class (lowest free slot first).

Pool slots are not inside the heap buffer, so pool allocations must not create heap blocks: `heap_stats` shows `used_blocks == 0` until a fallback or large request happens. Callers never pass a pointer from an earlier `classes_init`.

The hidden tests try the size boundaries 1, 16, 17, 64 and 65, exhaust each pool, check fallback, interior, double and foreign frees, slot reuse order, re-initialisation, and a seeded random mix in heaps of several sizes while verifying that no two live blocks overlap.
