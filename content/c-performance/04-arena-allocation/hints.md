The starter asks the system for every single block. Run it: 1000 nodes cost 1000 `sys_alloc` calls. Where should the pointer for the next node come from instead?
===
Keep `used` as the offset into the last chunk. To align, compute `off = (used + align - 1) & ~(align - 1)`. The chunk base is page aligned, so aligning the offset aligns the address.
===
If `nchunks == 0` or `off + n > size[nchunks-1]`, get a new chunk first. Only update `base`, `size`, `nchunks` and `used` after `sys_alloc` succeeded, so a failure leaves the arena unchanged.
