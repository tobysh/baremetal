The 32-byte `file_data` array and the descriptor table `fds` (4 slots, each with an `open` flag and an `offset`) are in the starter. Implement:

- `void files_init(void)` sets `file_data[i] = i` for `i` from 0 to 31 and closes every descriptor.
- `int file_open(void)` opens the **lowest** closed slot with offset 0 and returns its number, or returns -1 when all 4 slots are open.
- `int file_read(int fd, void *dst, size_t n)` copies up to `n` bytes from the descriptor's offset into `dst`, stopping at the end of the file, advances that descriptor's offset by the number copied and returns it. At end of file it returns 0; a zero-length read on a valid descriptor returns 0. It returns -1 for `fd` outside 0 to 3 or a closed descriptor.
- `int file_close(int fd)` closes an open descriptor and returns 0; it returns -1 for `fd` outside 0 to 3 or a descriptor that is already closed.

Every descriptor has its own offset. A reopened slot starts at offset 0.
