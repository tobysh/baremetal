In `pool_init`, compute `slot` and `count`, then loop `i` from 0 to `count - 1`: store in slot `i` a pointer to slot `i + 1` (and `NULL` in the last). Set `free_head` to slot 0 and `nfree = count`.
===
Use `memcpy(slot_ptr, &next, sizeof next)` or a `void **` cast to store and read the next pointer.
===
For `pool_free` validity: `ptr` must satisfy `base <= ptr < base + count * slot` and `(ptr - base) % slot == 0`. Do this on `uintptr_t` values so the comparison is well defined.
===
Double free: walk the free list from `free_head`; if you meet `ptr`, return -1. Only then push.
