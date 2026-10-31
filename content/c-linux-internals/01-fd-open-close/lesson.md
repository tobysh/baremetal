---
title: Descriptors and Open Files
summary: Model a process's descriptor table and the system-wide open file table, with lowest-free allocation and reference-counted close.
minutes: 25
difficulty: medium
tags: file-descriptors, kernel-model, state
id: c-linux-internals/fd-open-close
mode: harness
---
A file descriptor is just a small integer. The kernel keeps **two** tables behind it:

- the process's **descriptor table**, where `fd` is an index and each entry points to
- an entry in the system-wide table of **open file descriptions**, which holds the file, the current offset and a reference count.

```text
process table            system table
fd 0 ──────────────────▶ file 0: ino 7, offset 0,  refs 1
fd 1 ──────────────────▶ file 1: ino 9, offset 0,  refs 1
```

`open` takes the **lowest free** descriptor number (that is why a shell can predict which number a file gets) and creates a new description with offset 0. `close` removes the table entry and drops the reference count; only when it reaches zero is the description freed.

## The model, and what it leaves out

This is an in-memory model: `NFD = 8` descriptors per process and `NOFD = 8` descriptions. No real file is opened. Real Linux also has flags, permissions, `O_APPEND`, `errno` values, and limits that change at run time; none of that is modelled.

Run the starter to see the table dump after a close and reopen.

## Your turn

Implement `fd_open`, `fd_close` and `fd_advance`. The hidden tests include a full table, double closes, and a few thousand random operations compared with a reference that counts references instead of storing a count.
