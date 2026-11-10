Implement the two functions in the starter. The layout is in the lesson and in the comment above `Super`.

- `FormatSuper(dev, nblocks, ninodes)` writes zeros to the metadata blocks `0 .. DataStart-1`, then writes the superblock to block 0 as the **last** write. The unused tail of block 0 is zero. The data region (`DataStart ..`) is not touched, and neither are blocks at or beyond `nblocks`.
  - It must return an error **before writing anything** in these cases: `ErrBadSuper` if `nblocks` is larger than the device (`dev.Blocks()`) or larger than 512, or if `ninodes < 2`; `ErrNoSpace` if the metadata would leave no data block (`DataStart >= nblocks`). When both apply, `ErrBadSuper` wins.
  - `nblocks == DataStart + 1` (one data block) is valid.
- `ReadSuper(dev)` returns the decoded `Super`, or `ErrBadSuper` if any of these hold: wrong magic or version; `BitmapStart != 1` or `InodeStart != 2`; `NInodes < 2`; `NBlocks` larger than the device or 512; `DataStart != 2 + ceil(NInodes/2)`; or `DataStart >= NBlocks`. A disk of all zero bytes is therefore `ErrBadSuper`. `ReadSuper` never writes.

The hidden tests check the raw bytes on disk, many geometries, the error kinds, the write count after a rejected call, and corrupt each field in turn.
