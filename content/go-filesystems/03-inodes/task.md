Implement in the starter. `ino` is an inode number (slot 0 exists but `AllocInode` never hands it out). Inode numbers outside `0 .. NInodes-1` are `ErrRange`.

- `ReadInode(dev, sb, ino)` and `WriteInode(dev, sb, ino, in)` use the byte layout in the lesson: type, links, two zero bytes, size, six `Direct` words, all big-endian. `WriteInode` is exactly one block write and leaves the other inode of the pair unchanged; the 32 bytes of the slot are fully replaced (bytes 2-3 are zero).
- `WriteFile(dev, sb, ino, data)` replaces the contents following the four steps in the lesson, one bitmap write per allocated or freed block. `ErrNotFound` if the inode is free (`Type == 0`), `ErrTooBig` if `len(data) > MaxFile` (384), `ErrNoSpace` if the new blocks do not fit while the old ones are still allocated. After any of these errors the file reads back as before and the free-block count is unchanged. On success `Size` is `len(data)`, `Direct` lists the new blocks in order, unused entries are 0, and an empty `data` allocates nothing and frees the old blocks. Type and `Links` are kept.
- `FreeInode(dev, sb, ino)`: free every data block the inode owns, then zero the whole inode. `ErrNotFound` if it is already free.

The tests use raw bytes for the layout, a recording device for the order of writes, and block counting for leaks, over several file sizes and rewrite sequences.
