---
title: Non-Blocking I/O
summary: Make descriptors return immediately with EAGAIN instead of waiting, then drain readers and fill writers without ever getting stuck.
minutes: 30
difficulty: hard
tags: non-blocking, fcntl, sockets
id: c-networking/non-blocking
mode: harness
---
A normal `read` on an empty socket **sleeps** until data arrives. A normal `write` on a full socket sleeps until there is room. That is simple, but one slow peer can freeze your whole program.

Put a descriptor in **non-blocking** mode and the calls never sleep. Instead they fail at once with `errno` set to `EAGAIN` (also spelled `EWOULDBLOCK`, the same value on Linux), meaning "not now, try again later".

```c
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

Read the flags first and *add* the bit; writing `F_SETFL, O_NONBLOCK` alone would silently clear the others.

## What each call looks like now

- `read` returns data, `0` for end of file, or `-1` with `EAGAIN` ("nothing right now"). `EAGAIN` is **not** an error. It is the normal way to learn you have drained the socket.
- `write` may take *part* of your data, and then return `-1` with `EAGAIN` as soon as the buffer is full. You must remember what is left and send it later.
- `accept` on a non-blocking listening socket returns `-1` with `EAGAIN` when no client is waiting.

## Why bother

Combined with `poll()` ([Many Clients with poll()](/learn/c-networking/poll-hub)), non-blocking descriptors let one thread serve thousands of connections: poll says "this one is readable", you read until `EAGAIN`, move on. The write side adds a rule that trips people up: when a `write` is partial, keep the unsent tail and ask `poll` for `POLLOUT` to find out when to continue.

## This lesson

You will write the four building blocks: switch a descriptor to non-blocking, drain everything readable, push as much as fits, and accept a connection if one is waiting. If your code blocks, the test is killed after a few seconds, so a hang means you are still using a blocking call.
