`encoding/binary` works on a byte slice: `binary.BigEndian.PutUint32(b[4*i:], v)` is already wrapped as `putU32`, and `u32(b[4*i:])` reads one.
===
Build the 64-byte superblock in a fresh `make([]byte, BlockSize)` so the unused tail is zero, fill words 0..6, and write it last.
===
Do all the checks first. A failed `FormatSuper` must not have touched the device; the tests count writes.
===
`ReadSuper` compares `DataStart` with `2 + inodeBlocks(NInodes)`; the helper `inodeBlocks` is given. Check `NInodes` before you use it.
