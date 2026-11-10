---
title: A Bounded Pipe
summary: Move bytes through a circular pipe buffer and distinguish empty from EOF.
minutes: 45
difficulty: medium
tags: kernel, systems
id: osdev-2/pipes
mode: harness
---
`ls | grep txt` runs two programs at once and connects the standard output of the first to the standard input of the second. The connection is a **pipe**: a small buffer inside the kernel with a writing end and a reading end. Neither program knows the other exists; one writes bytes, the other reads them, and the kernel moves them through the buffer. Pipes were one of the defining ideas of Unix, and the same structure sits behind sockets, terminals and message queues.

This lesson builds the buffer and its rules. It brings together the [wait queue's](/learn/osdev-2/sleep-wake) ring buffer and a new idea: the difference between "no data **yet**" and "no data **ever again**".

## A ring of bytes

The pipe holds at most 16 bytes, in a ring exactly like the wait queue's: `head` is the index of the oldest unread byte and `count` is how many bytes are buffered. A write appends at `(head + count) % 16`; a read takes from `head` and advances it.

```text
After writing "abcdefghijklmnop" (16 bytes) and reading 10:

index: 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
     [ .   .   .   .   .   .   .   .   .   .   k   l   m   n   o   p ]
                                               ^head=10        count=6

Write "qrstuvwx" (8 bytes): slots (10 + 6) % 16 = 0, then 1 .. 7
     [ q   r   s   t   u   v   w   x   .   .   k   l   m   n   o   p ]
                                               ^head=10        count=14

A read of 24 bytes returns 14: "klmnop" then "qrstuvwx", in order.
```

No byte is ever shifted. That is the whole point of a ring: reads and writes cost only the bytes they move.

## Short transfers

A pipe is bounded, so a write may not fit and a read may ask for more than there is. Rather than fail, both **transfer what they can and return how many bytes moved**:

```text
pipe holds 0 bytes,  write 20 bytes  -> returns 16   (now full)
pipe is full,        write 1 byte    -> returns 0    (nothing fitted)
pipe holds 16 bytes, read 10 bytes   -> returns 10
pipe holds 6 bytes,  read 24 bytes   -> returns 6
```

This is exactly the contract of the Unix `read` and `write` system calls, and the reason every correct program calls them in a loop until the whole buffer is transferred. Code that assumes `write(fd, buf, n)` always writes `n` bytes works on files and breaks on pipes.

## Empty is not the end

When a reader finds the pipe empty, there are two very different situations:

```text
                      writer still open            writer closed
pipe has data         return the bytes             return the bytes   (drain first!)
pipe is empty         -1: try again later          0: end of file
```

An empty pipe with an open writer means the writer just hasn't produced anything yet; the reader should wait. An empty pipe whose writer has **closed** means nothing will ever arrive; the reader should stop. Returning 0 for "end of file" is the Unix convention: `while ((n = read(fd, buf, sizeof buf)) > 0)` is how every program reads to the end, and it only terminates because EOF is 0 and is distinct from "nothing yet".

In a blocking kernel, the -1 case would not return at all: the reader would `sleep_on` the pipe's wait queue and the writer would `wake_one` it after each write, all under the lock that protects the buffer (the lost-wakeup race again). This exercise is **nonblocking**, so the "try again" is reported as -1, like `EAGAIN` from a pipe opened with `O_NONBLOCK`.

Two details trip people up:

- **Closing does not discard data.** `printf` output that the writer produced just before exiting must still reach the reader. After `pipe_close_writer`, reads keep returning buffered bytes until the buffer is empty; only then do they return 0.
- **Zero-length requests** do not report data status. A read of 0 bytes returns 0 immediately, even when the pipe is empty and open, because nothing was asked for. A write of 0 bytes to an open pipe returns 0.

## Writing after close

Once the writer has closed its end, further writes are a bug in the caller. They return -1, including a zero-length write: the end itself is gone, so there is nothing to write *to*. (In Unix the opposite case, writing when the *reader* has closed, sends the writer a `SIGPIPE` signal. That is why `yes | head -1` terminates.)

## Initialisation

Pipes are created at run time, often inside freshly allocated memory, so `pipe_init` sets `head`, `count` and the closed flag explicitly. It does not need to clear the data array: bytes outside `[head, head + count)` are never read.

## Your turn

The `Pipe` type is in the starter. Implement the four operations with the rules above. The tests fill the pipe, wrap around its end, drain after close and check every return value.
