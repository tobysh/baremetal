---
title: Partial Reads and Writes
summary: read() and write() may move fewer bytes than you asked for, or be interrupted. Write read_full and write_full, the two loops every network program needs.
minutes: 25
difficulty: medium
tags: sockets, io, eintr
id: c-networking/partial-io
mode: harness
---
The most common bug in beginner network code looks like this:

```c
read(fd, buf, 100);       // "I asked for 100 bytes, so I have 100 bytes"
```

You do not. `read()` returns **whatever is available right now**, between 1 and the number you asked for. On a stream, a sender's single `write(fd, p, 100)` can arrive as 60 bytes and later 40. TCP is free to split and join the bytes however it likes.

`write()` has the mirror problem: when the kernel's send buffer is nearly full, it accepts only part of your data and returns a smaller count. Ignoring the return value silently drops the rest.

## The return values

For `read`:

- `n > 0`: that many bytes arrived;
- `n == 0`: **end of file**, the peer closed;
- `n == -1`: an error; check `errno`. The special case `EINTR` means a signal arrived while you were waiting; nothing is wrong, just call again.

For `write`: `n` bytes were taken (possibly fewer than requested), or `-1` with `errno`.

## The two loops

```c
size_t got = 0;
while (got < n) {
    ssize_t r = read(fd, (char *)buf + got, n - got);
    if (r == 0) break;                       // EOF: stop, report what we have
    if (r < 0) { if (errno == EINTR) continue; return -1; }
    got += r;
}
```

`write_full` is the same idea, with no EOF case. These helpers appear in practically every real network codebase, so write them carefully now. The tests deliver data in dribbles from another process, send a megabyte through a small socket buffer, and interrupt your calls with signals.
