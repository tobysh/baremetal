---
title: Exit Statuses, && and ||
summary: Decode a wait status the way a shell does (128 plus the signal number), then use it to implement ;, && and ||.
minutes: 30
difficulty: medium
tags: processes, shell
id: c-shell/exit-statuses
mode: harness
---
The integer that `waitpid` fills in is not an exit code. It is a small bundle of bits, and you must use the macros to read it.

| Question | Macro | Value |
| --- | --- | --- |
| Did it exit normally? | `WIFEXITED(s)` | then `WEXITSTATUS(s)` is 0 to 255 |
| Was it killed by a signal? | `WIFSIGNALED(s)` | then `WTERMSIG(s)` is the signal number |
| Was it stopped? | `WIFSTOPPED(s)` | then `WSTOPSIG(s)` (needs `WUNTRACED`) |

## The shell convention

A shell folds all of this into one number, the thing you see as `$?`:

- normal exit: the exit code itself;
- killed by signal *n*: **128 + n**. A `kill -9` victim gives 137, a segfault (signal 11) gives 139, Ctrl-C (signal 2) gives 130;
- command not found: 127, found but not executable: 126.

## Running lists

A command line can combine commands with these operators:

| Operator | Meaning |
| --- | --- |
| `a ; b` | run `a`, then always `b` |
| `a && b` | run `b` only if `a` succeeded (status 0) |
| `a \|\| b` | run `b` only if `a` failed (status not 0) |

The status of the whole list is the status of the **last command that actually ran**. `false && echo hi` never runs `echo`, and the list's status is the 1 from `false`.

This "short-circuit" is how `make && ./test` or `cd dir || exit 1` work in every script you will write.

## Your turn

The starter gives you `run_wait`, a ready-made fork/exec/wait that returns the raw wait status. Write the decoder and the list runner.
