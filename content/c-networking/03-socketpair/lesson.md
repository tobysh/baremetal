---
title: Sockets Without a Network
summary: socketpair() gives you two connected sockets in one process. Build a tiny request and reply exchange on it before touching TCP.
minutes: 20
difficulty: medium
tags: sockets, unix, ipc
id: c-networking/socketpair
mode: harness
---
A **socket** is a file descriptor you `read()` and `write()` like a file, except the bytes travel to somebody else. Before the complexity of addresses and ports, start with the simplest socket there is:

```c
int fds[2];
socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
```

You get two descriptors that are connected to each other. Bytes written to `fds[0]` come out of `fds[1]`, and the other way round. `AF_UNIX` means "a local socket", `SOCK_STREAM` means "a byte stream, like TCP". Everything you learn here about reading, writing and closing carries over unchanged to TCP.

## Streams have no message boundaries

Write 4 bytes twice and the reader might get 8 bytes in one `read()`, or 3 then 5. A stream is just bytes. That is why protocols need *framing*, which a later lesson covers. For now the messages are so small and the exchange so orderly that one `read` of 4 bytes returns all 4.

## End of file

When the other side calls `close()`, your `read()` returns **0**. That is not an error; it means "no more bytes will ever come". A server loop is therefore:

```c
while ((n = read(fd, buf, sizeof buf)) > 0) { ...handle...; }
```

and `n == 0` ends it cleanly, `n < 0` is an error.

## The exchange

Our protocol: the client sends a 4-byte big-endian number; the server replies with that number doubled, also 4 bytes big-endian. In the tests the server side runs `server_step` in a loop in a child process.
