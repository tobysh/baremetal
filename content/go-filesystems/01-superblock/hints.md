`encoding/binary` works on a byte slice. `putU32(b[4*i:], v)` writes word `i` and `u32(b[4*i:])` reads it. Both are already defined in the starter.
===
Build the superblock in a fresh `make([]byte, BlockSize)` so the unused tail is zero. Loop over the seven values `{magic, 1, nblocks, ninodes, 1, 2, start}` and write word `i` at offset `4*i`.
===
In `FormatSuper`, do every check first and `return` before the first `d.WriteBlock`. The tests compare `d.Writes` after a rejected call. Compare `nblocks` with `d.Blocks()` after converting to the same type, so a huge `uint32` cannot wrap.
===
Then write zero blocks for `0 .. start-1` and the superblock last. `ReadSuper` reads block 0 once, decodes the five fields after magic and version, and rejects if any of the conditions in the task is true. `DataStart` has to equal `2 + inodeBlocks(NInodes)`, and the helper `inodeBlocks` is given.
