Implement in the starter:

- `ReadInode(dev, sb, ino)` and `WriteInode(dev, sb, ino, in)` using the byte layout above. `ErrRange` for `ino < 0` or `ino >= NInodes`. `WriteInode` is exactly one block write and leaves the other inode of the pair alone.
- `WriteFile(dev, sb, ino, data)` following the four steps above. `ErrNotFound` if the inode is free, `ErrTooBig` over `MaxFile`, `ErrNoSpace` if the new blocks do not fit; in both failure cases the file and the free-block count are exactly as before. Unused `Direct` entries are 0.
- `FreeInode(dev, sb, ino)`: free every data block the inode owns, then zero the inode. `ErrNotFound` if it is already free.

The tests use raw bytes for the layout, a recording device for the order of writes, and block counting for leaks.
