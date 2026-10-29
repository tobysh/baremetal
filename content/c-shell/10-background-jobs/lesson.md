---
title: Background Jobs and Zombies
summary: Start a command without waiting for it, then reap it from a SIGCHLD handler so finished jobs never linger as zombies.
minutes: 35
difficulty: hard
tags: signals, processes, shell
id: c-shell/background-jobs
mode: harness
---
`sleep 30 &` returns the prompt at once. The trailing `&` means: fork the child as usual, but **do not wait** for it. The shell prints the job's pid and carries on.

## The zombie problem

When a child ends, the kernel keeps a small record of it (its pid and exit status) until the parent calls `wait`. A finished but unreaped child is a **zombie**: it uses no CPU or memory, but it fills the process table, and `ps` shows it as `<defunct>`. A shell that starts jobs in the background and never waits for them leaks one zombie per job.

## SIGCHLD

The kernel sends **SIGCHLD** to a parent whenever a child ends (or stops). By default it is ignored. Install a handler and reap there:

```c
static void on_sigchld(int sig) {
    (void)sig;
    int saved = errno;                         // handlers must not clobber errno
    while (waitpid(-1, NULL, WNOHANG) > 0)     // reap EVERY finished child
        reaped++;
    errno = saved;
}
```

Two details matter:

1. **Loop.** Signals do not queue. If three children exit at the same moment you may get a single SIGCHLD, so one `waitpid` call would leave two zombies. Loop until `waitpid` returns 0 (children exist but none finished) or -1 (none left).
2. **`WNOHANG`.** Without it, `waitpid` would block inside the handler as long as any child is still running.

Install the handler with `SA_RESTART` (so interrupted syscalls resume) and `SA_NOCLDSTOP` (do not signal merely because a child stopped).

`waitpid(-1, ...)` means "any child". That is also why a handler like this and a foreground `waitpid(pid)` in the same shell race: the handler may steal the foreground child's status. Real shells block SIGCHLD around foreground waits, or have the handler record statuses in a job table. Here you only build the background side.

## Your turn

The starter declares `volatile sig_atomic_t reaped`, a counter the tests read.
