Keep `static Region regions[4]; static int nregions;`. For `vm_region_add`, validate in this order: alignment and zero length, then `end = start + len` with a wrap check (`end < start`), then the range limits, then the overlap loop, then the table size. Store `end`, not `len`.
===
Overlap of `[a, b)` with `[c, d)` is `a < d && c < b`. Check against every registered region. Adjacent ranges (`b == c`) do not overlap and must be accepted.
===
In `pf_handle`, return 0 early for `err & 1` and `err & 16`. Find the region containing `addr` (`addr >= r->start && addr < r->end`); none means return 0. If `(err & 2) && !r->writable` return 0.
===
Then `uint64_t frame = frame_alloc(); if (!frame) return 0; memset((void *)(uintptr_t)frame, 0, 4096); map_page(addr & ~0xFFFull, frame, PTE_PRESENT | (r->writable ? PTE_WRITABLE : 0)); pages_faulted++; return 1;`. Zero the frame before mapping it, and count only after success.
===
```c
void pf_exception_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
    (void)rip;
    if (vector != 14) panic("unexpected exception");
    if (!pf_handle(read_cr2(), error_code)) panic("page fault outside any lazy region");
}
void vm_init(void) {
    nregions = 0;
    pages_faulted = 0;
    bm_exception_hook = pf_exception_handler;
}
```
