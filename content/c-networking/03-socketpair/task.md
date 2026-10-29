Implement three functions. The big-endian helpers are in the starter.

- `int make_channel(int fds[2])` creates a connected `AF_UNIX` / `SOCK_STREAM` pair with `socketpair`. Return `0` on success, `-1` on failure.
- `int server_step(int fd)` serves **one** request on `fd`: read a 4-byte big-endian number `n`, write back `n * 2` as 4 big-endian bytes (wrap around at 32 bits). Return `1` if it served a request, `0` if the peer had closed (`read` returned 0), `-1` on error.
- `int client_call(int fd, uint32_t x, uint32_t *result)` writes `x` as 4 big-endian bytes, reads the 4-byte reply into `*result` and returns `0`. Return `-1` on any error or if the connection closed first.

Assume each 4 byte message arrives in a single `read`. Handling the general case is the next lesson.
