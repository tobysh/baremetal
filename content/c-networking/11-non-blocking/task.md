Implement:

- `int set_nonblocking(int fd)` adds `O_NONBLOCK` to the descriptor's flags, **keeping the existing flags**. Return `0`, or `-1` on error.
- `ssize_t drain(int fd, char *buf, size_t cap)` reads from a non-blocking descriptor until one of: it would block (`EAGAIN`), `cap` bytes are stored, or end of file. Return the number of bytes stored (0 is possible when nothing is available). If the peer has closed **and no bytes were stored by this call**, return `-2`. Return `-1` on a real error. Retry on `EINTR`.
- `ssize_t push(int fd, const char *buf, size_t n)` writes from a non-blocking descriptor until all `n` bytes are written or it would block. Return how many bytes were accepted (possibly fewer than `n`, possibly 0), or `-1` on a real error. Retry on `EINTR`.
- `int try_accept(int listen_fd)` accepts a connection from a **non-blocking** listening socket. If a client is waiting, return the new descriptor **already set non-blocking**. If none is waiting, return `-2`. Return `-1` on a real error.

None of these may ever block.
