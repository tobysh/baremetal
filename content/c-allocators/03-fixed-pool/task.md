Using the `Pool` struct in the starter:

- `int pool_init(Pool *p, void *buf, size_t cap, size_t objsize)`: `buf` is 8-aligned. Slot size is `objsize` raised to at least 8 and rounded up to a multiple of 8. `count = cap / slot`. Build the free list through all slots in **ascending address order**, so the first allocations return slot 0, 1, 2 ... Return `count`. If `objsize` is 0, the slot size overflows or no slot fits, return 0 and make the pool permanently empty.
- `void *pool_alloc(Pool *p)`: pop a free slot, or `NULL` if none.
- `int pool_free(Pool *p, void *ptr)`: push the slot back. Return 0 on success. Return -1 and change nothing for `NULL`, a pointer outside the slots, a pointer that is not exactly at a slot start, or a slot that is already free.
- `size_t pool_available(const Pool *p)`: number of free slots.

Hidden tests replay seeded random alloc/free sequences (including bad frees) against a model that tracks which slots are live, check the exact LIFO order, and write a distinct pattern to every live slot to catch overlap.
