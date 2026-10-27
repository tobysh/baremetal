The shift for level `L` is `12 + 9 * (L - 1)`: level 1 shifts by 12, level 4 by 39. Then `(virt >> shift) & 0x1FF`.
===
Canonical check: shift right arithmetically by 47 as a signed number. `int64_t top = (int64_t)virt >> 47;` must be `0` or `-1`.
===
Walk with a loop: `table = cr3 & PTE_ADDR_MASK;` then for level 4 down to 1, `entry = ((uint64_t *)(uintptr_t)table)[pt_index(virt, level)]`. If `!(entry & PTE_PRESENT)` return `TRANSLATE_FAULT`. Update the permissions *before* deciding whether to continue.
===
At level 3 or 2, if `entry & PTE_HUGE`, stop. For level 2 the result is `(entry & 0x000FFFFFFFE00000) | (virt & 0x1FFFFF)`; for level 3 it is `(entry & 0x000FFFFFC0000000) | (virt & 0x3FFFFFFF)`. At level 1 the result is `(entry & PTE_ADDR_MASK) | (virt & 0xFFF)`.
===
Start `writable` and `user` as true (AND them with each entry) and `nx` as false (OR it in). Convert them to the flag bits only at the end.
