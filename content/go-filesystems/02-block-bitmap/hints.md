Read the bitmap block once with `dev.ReadBlock(int(sb.BitmapStart))`, change the byte, and write the block back. `bm[b/8] |= 1 << (b%8)` sets, `&^=` clears.
===
`GetBit` and `SetBit` are the primitives; `AllocBlock` and `FreeBlock` can either call them or do their own single read-modify-write. If you call `SetBit` after scanning, make sure the total is still one write.
===
Scan `DataStart` up to `NBlocks` (exclusive), never to the end of the 512 bits: the device may be larger than the filesystem.
===
Order the `FreeBlock` checks: range / metadata first (`ErrReserved`), then "already free" (`ErrNotAllocated`).
