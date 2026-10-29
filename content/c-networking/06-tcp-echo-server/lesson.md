---
title: A TCP Echo Server
summary: Create a listening socket on the loopback interface, accept a connection and echo bytes back until the client hangs up.
minutes: 25
difficulty: medium
tags: tcp, sockets, server
id: c-networking/tcp-echo-server
mode: harness
---
A TCP server follows the same five calls in nearly every program ever written:

```c
int s = socket(AF_INET, SOCK_STREAM, 0);          // 1. make a socket
bind(s, (struct sockaddr *)&addr, sizeof addr);   // 2. claim an address and port
listen(s, 8);                                     // 3. mark it as accepting connections
int c = accept(s, NULL, NULL);                    // 4. wait for one client; c is a NEW socket
read(c, ...); write(c, ...);                      // 5. talk, then close(c)
```

`accept` returns a **second** descriptor for that one client. The first, the *listening socket*, stays open to accept the next client.

## The address

`struct sockaddr_in` holds an IPv4 address and port, both in **network byte order** (that is why [Byte Order](/learn/c-networking/byte-order) came first):

```c
struct sockaddr_in a = {0};
a.sin_family = AF_INET;
a.sin_port = htons(port);                 // 0 means "pick any free port"
a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1: this machine only
```

If you pass port `0`, the kernel chooses one. Ask which with `getsockname()`, which fills in a `sockaddr_in` and its length (`socklen_t len = sizeof a;`).

## Reuse

Without `setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one)` set *before* `bind`, restarting a server can fail with "Address already in use" for a minute, while old connections sit in `TIME_WAIT`. Set it on every server.

## The echo loop

Read a chunk, write the same chunk back, repeat until `read` returns 0 (the client closed or called `shutdown(fd, SHUT_WR)`). Remember from [Partial Reads and Writes](/learn/c-networking/partial-io) that `write` can be partial, so the starter includes `write_full`.

Everything runs on 127.0.0.1 inside the sandbox; nothing leaves your machine. In the tests, the client connects and sends its data *before* your function runs (the kernel queues the connection in the backlog), so one process can play both sides.
