`ReadInode`/`WriteInode`: the block is `int(sb.InodeStart) + ino/InodesPerBlock`; the offset is `(ino%InodesPerBlock)*InodeSize`. The helper `inodePos` does this for you.
===
In `WriteInode`, zero the 32 bytes of your slot first, then fill them. Write the whole block back once.
===
`WriteFile`: compute `n = ceil(len(data)/BlockSize)`, allocate `n` blocks, and if one allocation fails free the ones you already got before returning the error.
===
Copy each chunk into a fresh zeroed `BlockSize` slice before writing it, so the tail of the last block is zero. Use `writeData`, not `dev.WriteBlock`, for data blocks (a later lesson relies on this).
===
`old := in` before you overwrite `in.Direct`; free `old.blocks()` entries only at the very end.
