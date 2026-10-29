---
title: Many Clients with poll()
summary: One thread, many connections. Use poll() to wait on a whole set of descriptors at once and build a chat hub that relays each message to everyone else.
minutes: 30
difficulty: hard
tags: poll, multiplexing, sockets
id: c-networking/poll-hub
mode: harness
---
A server that handles one client at a time, blocking in `read`, is stuck whenever that client is slow. You could start a thread per client, but there is a lighter tool: ask the kernel **which of my descriptors are ready**, and touch only those.

```c
struct pollfd p[3];
p[0].fd = a; p[0].events = POLLIN;
p[1].fd = b; p[1].events = POLLIN;
int ready = poll(p, 2, 1000);      // wait up to 1000 ms; -1 waits forever
if (ready > 0 && (p[0].revents & POLLIN)) { read(a, ...); }   // will not block
```

- `poll` fills in each entry's `revents` with what happened.
- The return value is the number of entries with events, `0` on timeout and `-1` on error (check `EINTR`).
- A descriptor whose `fd` is **negative is ignored**. That is how you "remove" a closed client from the array without shuffling it.
- When a peer closes, the descriptor shows up as readable (`POLLIN` and/or `POLLHUP`) and `read` returns 0. Always read to find out; do not test only `POLLIN`.

This is the heart of event-driven servers (the idea behind `select`, `epoll`, nginx and Node.js).

## The hub

You will write one step of a chat hub. The array `fds` holds the connections (or `-1` for an empty slot). One call to `hub_step`:

1. polls all open descriptors for input, for up to `timeout_ms`;
2. for each ready descriptor, does **one** `read` of up to 256 bytes;
3. if that read returned data, writes the same bytes to **every other** open descriptor; if it returned 0 or an error, closes that descriptor and sets its slot to `-1`.

Since every ready descriptor is read exactly once, a ready descriptor can never block you.
