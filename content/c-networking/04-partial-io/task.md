Implement two functions:

- `ssize_t read_full(int fd, void *buf, size_t n)` reads until `n` bytes have arrived. Returns the number of bytes read: `n` normally, or fewer **only** if the peer closed first (end of file; it may be 0). Returns `-1` on a real error. If a read is interrupted by a signal (`EINTR`), keep going.
- `ssize_t write_full(int fd, const void *buf, size_t n)` writes all `n` bytes, looping over short writes and retrying on `EINTR`. Returns `n` on success, `-1` on error. Writing `0` bytes returns `0`.
