`MemRegion`, `FRAME_SIZE`, `PMM_NONE`, `MMAP_USABLE` and `_kernel_end` are in the starter. Implement:

- `size_t mmap_parse(const void *mb_info, MemRegion *out, size_t max)` reads the Multiboot memory map. It returns 0 if flag bit 6 is clear. Otherwise it copies up to `max` entries into `out` (advancing by each entry's `size + 4`) and returns how many it stored.
- `void pmm_init(uint8_t *bitmap, size_t frame_count)` uses `bitmap` (at least `(frame_count + 7) / 8` bytes) and marks every frame **used**.
- `void pmm_add_region(const MemRegion *r)` frees every frame that lies entirely inside `r`, but only if `r->type == MMAP_USABLE`. Frames beyond `frame_count` are ignored.
- `void pmm_reserve(uint64_t base, uint64_t length)` marks every frame the range touches (even partly) as used. A length of 0 reserves nothing.
- `uint64_t pmm_alloc(void)` marks the lowest free frame used and returns its address, or `PMM_NONE` if there is none.
- `int pmm_free(uint64_t addr)` frees the frame at `addr` and returns 0. It returns -1 for an address that is not frame-aligned, out of range, or already free.
- `size_t pmm_free_count(void)` returns how many frames are free.
- `void pmm_setup(const void *mb_info, uint8_t *bitmap, size_t frame_count)` calls `pmm_init`, adds every region of the memory map, then reserves the first MiB and the kernel image (`0x100000` up to `_kernel_end`).
