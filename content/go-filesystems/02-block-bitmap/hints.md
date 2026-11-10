A bit is `bm[b/8] & (1 << uint(b%8))`. Read block 1 with `dev.ReadBlock(int(sb.BitmapStart))`, change the byte, and write the whole block back. Use `|=` to set and `&^=` to clear.
===
`GetBit` and `SetBit` are the primitives. Check the range first and return before touching the device. `AllocBlock` and `FreeBlock` may call them, but count the writes: a scan that only calls `GetBit` costs none, and the single `SetBit` at the end costs one.
===
Scan from `DataStart` up to `NBlocks` exclusive, never to 512: the bitmap has more bits than the filesystem has blocks, and the device may be larger than the filesystem.
===
`MarkMetadata` should build a brand-new zeroed 64-byte slice and set bits into it, rather than editing what is on the device, so stale bits disappear. In `FreeBlock` check the range first (`ErrReserved`), then whether the bit is already clear (`ErrNotAllocated`).
