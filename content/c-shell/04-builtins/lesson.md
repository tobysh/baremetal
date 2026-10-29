---
title: Builtins - cd and exit
summary: Some commands cannot be separate programs. Learn why cd and exit must run inside the shell itself, and dispatch them before forking.
minutes: 25
difficulty: medium
tags: processes, shell
id: c-shell/builtins
mode: harness
---
Try this thought experiment. Your shell forks a child and the child runs a program called `cd` that calls `chdir("/tmp")`. The child's working directory changes, then the child exits. The shell's own working directory is untouched. The same goes for `exit`: a child can only end itself.

Anything that must change the **shell's own state** (its directory, its variables, whether it is still running) has to be a **builtin**: ordinary C code run by the shell process itself, with no fork.

## Dispatch before fork

```c
if (is_builtin(argv[0])) run_it_here();
else fork_and_exec();
```

A builtin receives the same `argv` as any program. Compare `argv[0]` with `strcmp`.

## cd

`chdir(path)` changes the process working directory and returns `0` or `-1`. With no argument `cd` goes to `$HOME` (use `getenv`). On failure a shell prints a message on **standard error** (`perror`-style: `cd: /nope: No such file or directory`) and sets the status to 1.

## exit

`exit` stops the shell. `exit 3` stops it with status 3. A bare `exit` uses the status of the last command. Because the number must fit in one byte, take it modulo 256. The builtin should not call `exit()` itself in this design: it reports "please quit" to the main loop, which cleans up first. That keeps the builtin easy to test.

## Status

Every command, builtin or not, leaves a **status**: `0` for success, nonzero for failure. The shell keeps the last one (you will see it as `$?` later). Builtins update it too, through a pointer.

## Your turn

Write `run_builtin`, the function a shell calls first for every parsed command.
