---
title: Connecting Two Commands with a Pipe
summary: Create a pipe, give each child one end with dup2, and close every other copy so the reader sees end-of-file.
minutes: 40
difficulty: hard
tags: processes, pipes, shell
id: c-shell/pipe-two
mode: harness
---
`echo hello | tr a-z A-Z` runs two programs at once and feeds the output of the first into the input of the second. The kernel object that carries the bytes is a **pipe**.

## pipe

```c
int fds[2];
pipe(fds);   // fds[0] is the read end, fds[1] is the write end
```

Bytes written to `fds[1]` come out of `fds[0]`, in order, through a buffer in the kernel. If the buffer is empty, a read **blocks**. If the buffer is full, a write blocks.

## dup2: rewire stdin and stdout

Programs read from file descriptor 0 and write to descriptor 1 without knowing what they are connected to. `dup2(old, new)` makes descriptor `new` refer to the same thing as `old`. So in the left child:

```c
dup2(fds[1], STDOUT_FILENO);   // its stdout is now the pipe
close(fds[0]); close(fds[1]);  // the originals are no longer needed
execvp(left[0], left);
```

and in the right child `dup2(fds[0], STDIN_FILENO)`.

## The rule that causes every pipe bug

A reader only sees end-of-file when **every** copy of the write end is closed. After `fork`, the parent and both children each hold copies of both ends. If *any* process still has the write end open (and the parent forgetting is the classic culprit), `tr` will wait for more input forever. So:

1. In each child, close both pipe ends after the `dup2`.
2. In the parent, close both ends right after forking both children.
3. Then `waitpid` for both children.

The reverse also matters. If the reader quits early (`yes | head -n 1`), the writer gets **SIGPIPE** when it writes into a pipe with no readers, which ends it politely. That only works if the parent has closed its own read end.

## Where the last command writes

A shell often needs the output of a pipeline to go somewhere other than its own stdout (redirection, as you will see soon). Here the final destination is a parameter, `out_fd`.

## Your turn

Write `run_pipe2`.
