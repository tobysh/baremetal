The `PTE_*` flag constants, `TRANSLATE_FAULT` and `read_cr3()` are in the starter. Implement:

- `unsigned pt_index(uint64_t virt, int level)` returns the 9-bit table index for `level` (4 = PML4 down to 1 = PT).
- `uint64_t translate(uint64_t cr3, uint64_t virt, uint64_t *perms)` walks the tables starting at the physical address in `cr3` (ignore its low 12 bits) and returns the physical address `virt` maps to, or `TRANSLATE_FAULT` if the address is non-canonical or any entry on the walk is not present.
  - A level-3 entry with `PTE_HUGE` maps 1 GiB; a level-2 entry with `PTE_HUGE` maps 2 MiB.
  - If `perms` is not `NULL`, store the effective permissions: `PTE_WRITABLE` and `PTE_USER` are set only if **every** entry on the walk has them, and `PTE_NX` is set if **any** entry has it. Store 0 for the other bits.
