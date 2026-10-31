---
title: dup, dup2 and Redirection
summary: Several descriptors can name one open file description and share its offset. dup2 is how a shell redirects output; the reference count decides when it is freed.
minutes: 30
difficulty: medium
tags: file-descriptors, kernel-model, dup2, redirection
id: c-linux-internals/dup-and-redirect
mode: harness
---
Lesson 1 gave every descriptor its own description. That is what happens when you `open` a file twice. `dup` is different: it adds a **second name for the same description**.

```text
fd 1 ──┐
       ├─▶ file 0: ino 555, offset 10, refs 2
fd 4 ──┘
```

Because the offset lives in the description, a write through fd 1 moves the position seen through fd 4. Two separate `open`s of the same path get two descriptions and two offsets.

## dup2 and shell redirection

`dup2(old, new)` makes `new` refer to `old`'s description, **closing `new` first** if it was open. A shell implements `cmd > out.txt` as: open `out.txt` (say fd 2 is the result), `dup2(2, 1)`, `close(2)`. The old stdout description loses its last name and is freed; the file is now reached only through fd 1.

Edge cases that real programs rely on: `dup2(fd, fd)` does nothing and returns `fd` if `fd` is valid (and fails otherwise); a failed `dup2` must not close anything.

## Model limits

Same model as lesson 1. The close-on-exec flag, `fork` and `fcntl` are not modelled.

## Your turn

Implement `fd_dup` and `fd_dup2`. `fd_open`, `fd_close` and `fd_advance` are given. Hidden tests replay thousands of random steps against a reference that counts references.
