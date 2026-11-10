`ReadInode` and `WriteInode`: the block is `int(sb.InodeStart) + ino/InodesPerBlock` and the offset is `(ino%InodesPerBlock)*InodeSize`. The helper `inodePos` returns both. Check `ino` against `NInodes` first.
===
In `WriteInode`, read the whole block, clear your 32-byte slot, fill it field by field with `putU32`, and write the block back once. The other inode in the pair shares those bytes, so never rewrite the block from scratch.
===
`WriteFile`: compute `n = (len(data)+BlockSize-1)/BlockSize`. Refuse `ErrTooBig` and `ErrNoSpace` before changing anything (`CountFree` tells you if `n` blocks exist), or free what you already took if a later `AllocBlock` fails. Copy each chunk into a fresh zeroed `BlockSize` slice and write it with `writeData`, so the tail of the last block is zero.
===
Keep a copy of the old inode (`old := in`) before you overwrite `in.Direct`, write the new inode, and free `old.blocks()` only at the very end. `FreeInode` frees the blocks first and zeroes the inode after, using the `Inode{}` zero value.
