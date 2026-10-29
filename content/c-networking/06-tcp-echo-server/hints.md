`tcp_listen`: `socket`, then `setsockopt(... SO_REUSEADDR ...)`, fill a `sockaddr_in` with `htons(port)` and `htonl(INADDR_LOOPBACK)`, then `bind`, `listen`, and `getsockname` to learn the port. Convert it back with `ntohs`.
===
Call `getsockname` with `socklen_t len = sizeof addr;` and `(struct sockaddr *)&addr`. Check every call; on a failure `close(s)` before returning -1.
===
```c
long total = 0;
char buf[1024];
ssize_t n;
while ((n = read(c, buf, sizeof buf)) > 0) {
    if (write_full(c, buf, n) < 0) { close(c); return -1; }
    total += n;
}
close(c);
return n < 0 ? -1 : total;
```
