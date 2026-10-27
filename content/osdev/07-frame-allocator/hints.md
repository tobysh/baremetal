The multiboot info is just bytes: `const uint8_t *p = mb_info;` then read a 32-bit field with `*(const uint32_t *)(p + 44)`. An entry's base is at `entry + 4`, length at `entry + 12`, type at `entry + 20` (all unaligned-safe on x86). Loop while `offset < mmap_length`, adding `size + 4` each time.
===
Keep `static uint8_t *bm; static size_t nframes, nfree;` and small helpers `is_used(i)`, `set_used(i)`, `set_free(i)` that use `bm[i / 8]` and the bit `1 << (i % 8)`. Only change `nfree` when a bit actually flips, so adding the same region twice can't corrupt the count.
===
For `pmm_add_region`: `first = (base + 4095) / 4096` (round up) and `end = (base + length) / 4096` (round down). Free frames `first` up to but not including `min(end, nframes)`.
===
For `pmm_reserve`: `first = base / 4096` (round down) and `end = (base + length + 4095) / 4096` (round up). Return early if `length == 0`.
===
`pmm_free` must reject a frame that is already free; that's how a double free is caught instead of silently inflating the free count.
===
`pmm_setup` is a loop over `mmap_parse` results, then two calls to `pmm_reserve`: `(0, 0x100000)` and `(0x100000, (uint64_t)(uintptr_t)_kernel_end - 0x100000)`.
