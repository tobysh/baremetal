Walk levels 4, 3 and 2 in a loop. For each one take a pointer to the entry: `uint64_t *e = &table_at(table)[pt_index(virt, level)];`. Then handle three cases: present and huge (return -1), present (go on), not present (allocate).
===
For a missing table: `uint64_t frame = alloc_frame(); if (!frame) return -1; *e = frame | PTE_PRESENT | PTE_WRITABLE | PTE_USER;`. Then set `table = *e & PTE_ADDR_MASK;` for the next level, in both the present and the new case.
===
After the loop, `table` is the physical address of the PT. The leaf is `&table_at(table)[pt_index(virt, 1)]`. Refuse if it is already present; otherwise write `phys | flags | PTE_PRESENT`.
===
Mask the flags first: `flags &= PTE_WRITABLE | PTE_USER | PTE_NX;`. Without that, a caller passing `PTE_HUGE` or garbage would corrupt the entry.
===
`unmap_page` is the same walk without allocating: any non-present or huge entry means -1. Then `*leaf = 0; tlb_invalidate(virt);`. If the page still seems to work after unmapping, you forgot the flush.
