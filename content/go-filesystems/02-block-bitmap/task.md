Using the helpers you already have (`ReadSuper`, `Device`), implement these. `sb` is the `Super` returned by `ReadSuper`. Block numbers are `int`.

- `GetBit(dev, sb, b)` and `SetBit(dev, sb, b, used)`: `ErrRange` (and no write) unless `0 <= b < NBlocks`. `SetBit` always does exactly one write, even if the bit already has that value. Both work for metadata blocks too.
- `MarkMetadata(dev, sb)`: sets bits `0 .. DataStart-1` and clears every other bit of the bitmap block (stale bits too), in a single write.
- `AllocBlock(dev, sb)`: returns the lowest free block in `DataStart .. NBlocks-1` and marks it used with one write. When none is left it returns `ErrNoSpace` and does not write.
- `FreeBlock(dev, sb, b)`: `ErrReserved` if `b` is not in `DataStart .. NBlocks-1` (this includes negative numbers and `b >= NBlocks`), `ErrNotAllocated` if its bit is already 0, otherwise it clears the bit with one write. Failed calls do not write.
- `CountFree(dev, sb)`: the number of free blocks in `DataStart .. NBlocks-1`. It does not write.

Bit `b` is byte `b/8`, position `b%8`, least significant bit first. The tests check raw bitmap bytes, and compare your bitmap with a simple `[]bool` model over hundreds of random operations.
