Using the helpers you already have (`ReadSuper`, `Device`), implement:

- `GetBit(dev, sb, b)` and `SetBit(dev, sb, b, used)`: `ErrRange` outside `0 .. NBlocks-1`. `SetBit` always does one write.
- `MarkMetadata(dev, sb)`: sets bits `0 .. DataStart-1`, clears the rest, in a single write.
- `AllocBlock(dev, sb)`: the lowest free block in `DataStart .. NBlocks-1`, marked used with one write; `ErrNoSpace` (and no write) when none is left.
- `FreeBlock(dev, sb, b)`: `ErrReserved` if `b` is not a data block, `ErrNotAllocated` if its bit is already 0, otherwise clears it with one write.
- `CountFree(dev, sb)`: the number of free data blocks.

The tests compare your bitmap with a simple `[]bool` model over hundreds of random operations.
