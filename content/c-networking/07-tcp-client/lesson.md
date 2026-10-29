---
title: A TCP Client
summary: Connect to a server on loopback, send a one-line request and read a one-line reply that may arrive in pieces.
minutes: 25
difficulty: medium
tags: tcp, sockets, client
id: c-networking/tcp-client
mode: harness
---
A client is the server's mirror with fewer steps. It does not `bind` or `listen`; it just makes a socket and `connect`s:

```c
int s = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in a = {0};
a.sin_family = AF_INET;
a.sin_port = htons(port);
a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
```

`connect` performs the TCP three-way handshake. It fails with `ECONNREFUSED` if nothing is listening on that port. The kernel picks the client's own port (the *ephemeral* port) automatically.

## Request and reply

Our tiny protocol is line-based ([Framing a Line Protocol](/learn/c-networking/line-framing)): the client sends one line ending in `\n`, and the server answers with one line ending in `\n`.

The reply may arrive in several pieces, so one `read` is not enough. Keep reading and appending until you have seen the `\n`, or the server closes. Reading **one byte at a time** is the simplest correct way to stop exactly at the newline without consuming bytes that belong to the *next* reply. It is slower than buffering, but fine for a short line, and real clients usually add a buffer like the `LineBuf` you wrote.

## Cleaning up

Close the socket on every path, including the error paths. A leaked descriptor in a long-running client is a bug that shows up hours later as "Too many open files".

In the tests, a child process plays the server, and it deliberately sends the reply a couple of bytes at a time.
