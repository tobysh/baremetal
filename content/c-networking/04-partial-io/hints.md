Keep a running count `done`. Each time through the loop, call `read(fd, (char *)buf + done, n - done)`; advance the pointer by what you have already done, and ask only for what is left.
===
Check `r == 0` (EOF, break out and return `done`) before `r < 0`. For `r < 0`, `if (errno == EINTR) continue;` otherwise return -1.
===
```c
ssize_t write_full(int fd, const void *buf, size_t n) {
    size_t done = 0;
    while (done < n) {
        ssize_t w = write(fd, (const char *)buf + done, n - done);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        done += (size_t)w;
    }
    return (ssize_t)done;
}
```
