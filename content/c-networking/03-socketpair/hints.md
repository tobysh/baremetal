`make_channel` is one line: `return socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0 ? 0 : -1;`.
===
`server_step`: `uint8_t buf[4]; ssize_t n = read(fd, buf, 4);` If `n == 0` return 0, if `n != 4` return -1. Then `put_u32(buf, get_u32(buf) * 2)` and `write(fd, buf, 4)`.
===
`client_call` is the mirror image: `put_u32(buf, x); write(fd, buf, 4);` then `read(fd, buf, 4)` and `*result = get_u32(buf)`. Check that both return exactly 4.
