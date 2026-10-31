Implement the two functions in the starter. Block numbers and layout are in the comment above `Super`.

- `FormatSuper(dev, nblocks, ninodes)` writes zeros to the metadata blocks `0 .. DataStart-1` and then the superblock. It leaves the data region untouched. It returns an error, **before writing anything**, if `nblocks` exceeds the device or 512, if `ninodes < 2`, or if the metadata would leave no data block (`DataStart >= nblocks`). The last one is `ErrNoSpace`; the others `ErrBadSuper`.
- `ReadSuper(dev)` returns the decoded `Super`, or `ErrBadSuper` if the magic, version, `BitmapStart`, `InodeStart` or `DataStart` are wrong, if `NInodes < 2`, or if `NBlocks` exceeds the device or 512.

Hidden tests check the raw bytes on disk, many geometries, and corrupt each field in turn.
