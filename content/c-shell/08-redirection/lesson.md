---
title: Redirection with dup2
summary: Handle <, >, >> and 2> by opening files in the child and wiring them onto descriptors 0, 1 and 2 with dup2, before exec.
minutes: 35
difficulty: hard
tags: processes, file-descriptors, shell
id: c-shell/redirection
mode: harness
---
In `sort < in.txt > out.txt`, the program `sort` has no idea files are involved. It reads descriptor 0 and writes descriptor 1. The shell arranged the plumbing between `fork` and `exec`.

## Descriptors and dup2

Every process has a table of open files. Entries 0, 1 and 2 are **stdin**, **stdout** and **stderr**. `open` returns the lowest free slot. `dup2(from, to)` closes `to` if open and makes it a copy of `from`. The trick:

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
dup2(fd, STDOUT_FILENO);   // fd 1 now points at out.txt
close(fd);                 // the extra descriptor is no longer needed
execvp(argv[0], argv);     // the new program inherits descriptor 1
```

Open files survive `exec`; that is exactly why this works.

## The four redirections

| Syntax | Meaning | `open` flags |
| --- | --- | --- |
| `< file` | stdin from file | `O_RDONLY` |
| `> file` | stdout to file, emptied first | `O_WRONLY \| O_CREAT \| O_TRUNC` |
| `>> file` | stdout appended | `O_WRONLY \| O_CREAT \| O_APPEND` |
| `2> file` | stderr to file | like `>` |

New files are created with mode `0666`; the process `umask` trims it.

## Do it in the child

If the shell did the `dup2` itself it would lose its own stdout. Do the redirection in the child, after `fork` and before `exec`. If a file cannot be opened, the child prints `sh: <file>: <reason>` to stderr and exits with status **1** without running the command.

## Stripping the redirection words

The redirection operators are **not** arguments. `echo hi > f` must run `echo` with just `hi`. Walk `argv`, and each time you meet `<`, `>`, `>>` or `2>`, take the next word as the filename and leave both out of the new argv. If the operator is the last word (no filename), it is a syntax error: print `sh: syntax error` to stderr and return status 2 without running anything. Later redirections of the same stream win (`> a > b` creates both and writes to `b`).

## Your turn

Write `run_redirect`. The argv array is passed in with the redirection words still inside; you may build a cleaned copy.
