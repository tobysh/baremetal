---
title: Review: File Offsets and Bounded Storage
summary: Combine bounded storage and descriptor offsets in a small file table.
minutes: 40
difficulty: medium
tags: kernel, systems
id: osdev-2/review-file-offsets
mode: harness
kind: review
---
This review puts two earlier ideas side by side: the overflow-safe bounds checks from the [RAM disk](/learn/osdev-2/ramdisk), and the "transfer what you can, report how much" contract from [pipes](/learn/osdev-2/pipes). The new piece that joins them is the **file descriptor** and the **offset** it carries. Nothing here needs a new mechanism; it is about getting familiar rules right in a slightly different arrangement.

## Where the offset lives

When a program calls `read(fd, buf, 100)` it does not say *where* in the file to read. The kernel remembers that for it. Each `open` creates an **open file description** holding the current offset (plus flags and permissions in a real kernel), and the integer the program gets back, the **file descriptor**, is an index into a per-process table of them.

The important consequence: the offset belongs to the **open**, not to the file. Opening the same file twice gives two descriptors with two independent offsets:

```text
descriptor table                     the file (32 bytes: 0, 1, 2, ... 31)

fd 0  [ open,   offset 8 ] --+
fd 1  [ open,   offset 3 ] --+--> [ 00 01 02 03 04 05 06 07 08 09 ... 1F ]
fd 2  [ closed           ]                   ^              ^
fd 3  [ closed           ]                   fd 1 next      fd 0 next
```

Reading through fd 0 moves only fd 0's offset. That is why two programs can each `cat` the same file at the same time without interfering. (Descriptors that **share** an offset exist too: `dup(fd)` and `fork()` make a second descriptor that points at the *same* open file description. This model leaves sharing out; every `file_open` creates a fresh description.)

## Choosing a descriptor number

`file_open` returns the **lowest** free slot. POSIX requires this, and real programs rely on it: the classic way to redirect standard output was `close(1); open("out.txt", ...)`, trusting that the new file lands in slot 1. When all four slots are in use, `file_open` returns -1 (Unix would report `EMFILE`, too many open files).

A closed slot is free again and is reused with **a fresh offset of zero**. Nothing from the previous user of the slot may leak into the new open.

## Reading with an offset

A read of `n` bytes at offset `off` in a 32-byte file can move at most `32 - off` bytes. Clamp, copy, advance:

```text
fd at offset 30, file_read(fd, buf, 5):
    left = 32 - 30 = 2        n = min(5, 2) = 2
    copy bytes 30, 31         offset becomes 32       returns 2   (a short read)

same fd, file_read(fd, buf, 5):
    left = 32 - 32 = 0        n = 0                   returns 0   (end of file)
```

As with pipes, a short read is not an error, and 0 means end of file. The difference is that a file at EOF never gets more data in this model, so there is no "try again" case.

The offset is always between 0 and 32, because it only ever grows by at most `32 - offset`. That is the RAM-disk trick turned around: since `offset <= 32` is guaranteed, `32 - offset` cannot underflow, and nothing is ever added before it is checked. A zero-length read on a valid descriptor returns 0 and moves nothing.

## Validating descriptors

The descriptor is an integer that came from a user program, so it must be checked before it is used as an array index:

```c
if (fd < 0 || fd >= 4 || !fds[fd].open)
    return -1;                  /* EBADF in Unix */
```

All three conditions matter. A negative `fd` would index before the array; `fd >= 4` past it; and a closed slot must be refused so that reading through a stale descriptor (one the program already closed) cannot quietly continue. `file_close` uses the same check, which also makes a double close return -1.

## A full trace

```text
files_init()                  file = 0..31, all slots closed
x = file_open()     -> 0
y = file_open()     -> 1
file_read(x, a, 5)  -> 5      a = 0 1 2 3 4        fd 0 offset 5
file_read(y, b, 3)  -> 3      b = 0 1 2            fd 1 offset 3 (independent)
file_read(x, a, 40) -> 27     a = 5 .. 31          fd 0 offset 32
file_read(x, a, 1)  -> 0      end of file
file_close(y)       -> 0
file_read(y, b, 1)  -> -1     closed descriptor
file_close(y)       -> -1     already closed
file_open()         -> 1      lowest free slot, offset 0 again
```

## Your turn

The file's 32 bytes and the descriptor table are in the starter. Implement the four functions. The tests open and close slots in different orders, read through EOF and use invalid descriptors.
