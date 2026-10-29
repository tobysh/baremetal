`set_nonblocking`: `int fl = fcntl(fd, F_GETFL, 0); if (fl < 0) return -1; return fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0 ? -1 : 0;`.
===
`drain` is a loop: `r = read(fd, buf + got, cap - got)`. For `r > 0` add to `got`; for `r == 0` set a flag and break; for `r < 0` check `errno`: `EINTR` continue, `EAGAIN` / `EWOULDBLOCK` break, anything else return -1. At the end `if (eof && got == 0) return -2;`.
===
`push` is the same shape with `write`. Stop when the loop has written everything or `errno == EAGAIN`. `try_accept`: if `accept` returns `-1` and `errno` is `EAGAIN` or `EWOULDBLOCK`, return -2; otherwise call `set_nonblocking` on the new descriptor (close it if that fails).
