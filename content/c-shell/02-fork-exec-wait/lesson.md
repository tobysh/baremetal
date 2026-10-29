---
title: fork, exec and wait
summary: The heart of every shell. Clone the process with fork, replace the clone with a new program using exec, and collect its exit code with waitpid.
minutes: 30
difficulty: medium
tags: processes, shell, syscalls
id: c-shell/fork-exec-wait
mode: harness
---
A shell never runs `ls` itself. If it did, `ls` would replace the shell and you would be logged out when it finished. Instead Unix splits "start a program" into two separate steps.

## fork: make a copy

`fork()` duplicates the calling process. It returns **twice**: in the original (the *parent*) it returns the child's process id, and in the new copy (the *child*) it returns `0`. On failure it returns `-1`.

```c
pid_t pid = fork();
if (pid < 0) { /* could not fork */ }
else if (pid == 0) { /* I am the child */ }
else { /* I am the parent; pid is the child's id */ }
```

## exec: become another program

The `exec` family replaces the current process image with a new program. If it works, **it never returns**, because the code that called it no longer exists. `execvp(file, argv)` takes the `argv` array (ending in `NULL`) and looks `file` up in `PATH`:

```c
execvp(argv[0], argv);
// only reached if exec failed
_exit(127);
```

After a failed exec the child must not carry on running the parent's code. Leave with `_exit` (not `exit`, which would flush the parent's buffered output a second time). By convention 127 means "command not found".

## waitpid: collect the result

The parent calls `waitpid(pid, &status, 0)` to sleep until the child ends. The `status` integer packs several facts; the macros unpack it:

```c
if (WIFEXITED(status))   code = WEXITSTATUS(status);   // normal exit, 0 to 255
if (WIFSIGNALED(status)) sig  = WTERMSIG(status);      // killed by a signal
```

Until the parent waits, a finished child stays in the process table as a **zombie**, holding its exit status. Always reap your children. If `waitpid` is interrupted by a signal it returns `-1` with `errno == EINTR`, so retry in a loop.

## Your turn

Write `run_command`, the function every later lesson builds on.
