Implement three functions on `sysfiles *s` and `fdtable *t`:

- `fd_open(s, t, ino)`: use the lowest free descriptor number and the lowest free description (`refs == 0`). Set it to `{ino, offset 0, refs 1}`. Return the descriptor, or `-1` if either table is full, leaving everything unchanged.
- `fd_close(s, t, fd)`: return `-1` for a number outside `0..NFD-1` or one that is not open. Otherwise clear the table entry, decrement `refs`, and when it reaches 0 reset the whole description to zeros. Return 0.
- `fd_advance(s, t, fd, n)`: add `n` to the description's offset and return the new offset. Return `-1` for a bad `fd` or negative `n`.

Do not change the structure definitions; the tests read them directly.
