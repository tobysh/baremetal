`aspace_create`: `uint64_t pml4 = frame_alloc(); if (!pml4) return 0; ((uint64_t *)pml4)[0] = ((uint64_t *)kernel_pml4)[0]; return pml4;`. The frame comes back zeroed, so entries 1 to 511 are already empty.
===
`aspace_alloc_page`: check the address first (`virt & 0xFFF`, `virt < USER_BASE`, `virt >= USER_END`), then `frame_alloc()`, then `map_page(as, virt, frame, PTE_U | (writable ? PTE_W : 0))`. If `map_page` fails, call `frame_free(frame)` and return 0.
===
`aspace_copy_to`: first reject `virt < USER_BASE || len > USER_END - virt`. Then loop over every page the range touches (`for (a = virt & ~0xFFFULL; a < virt + len; a += 4096)`) calling `leaf_pte`, and fail if any entry is missing or not present. Only then do a second loop that copies chunk by chunk: `chunk = min(len, 4096 - (virt & 4095))`, destination `(pte & ADDR_MASK) + (virt & 4095)`.
===
`aspace_destroy` is a recursive helper: `free_tree(table, level)`. At level 0 the entries point to data frames: free those. At other levels recurse with `level - 1`. Then free the table itself. Call it for each present PML4 entry from 1 up, with the PDPT as level 2 (PDPT, PD, PT, data), and count every `frame_free`.
===
If the machine triple-faults after `aspace_switch`, the new PML4's entry 0 is wrong: the CPU cannot even fetch the next instruction. If `bad_frees` is not zero, you freed something that did not come from `frame_alloc`, probably the kernel's own tables through PML4 entry 0.
