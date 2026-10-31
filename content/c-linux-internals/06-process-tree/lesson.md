---
title: Process Trees, Zombies and Reparenting
summary: Model fork, exit and wait in a fixed process table: zombies keep their exit status until reaped, and orphans are adopted by init.
minutes: 35
difficulty: medium
tags: processes, zombies, kernel-model
id: c-linux-internals/process-tree
mode: harness
---
Every process except init (pid 1) has a parent. This lesson models the part of process management that decides **who is left holding what**.

```text
init(1) ── shell(2) ── job(3) ── helper(4)

job exits with status 3:
init(1) ── shell(2) ── job(3, zombie)
        └── helper(4)            (adopted by init)
```

- **fork** creates a running child of the caller.
- **exit** does not free the slot. The process becomes a **zombie** that keeps only its pid, parent and exit status, so the parent can still learn how it ended. Its children are **reparented to init**.
- **wait** by the parent returns a zombie child's pid and status and then **frees** the slot. A parent that never waits leaves zombies behind.

You saw `waitpid` in the shell track; here you build the table behind it.

## Return values

`pt_wait` distinguishes three cases that look alike but need different caller behaviour: a reaped pid (> 0), "children exist but none has exited yet" (0, a real `wait` would block), and "no children at all" (-1, `ECHILD`).

## Model limits

Pids are small and reused lowest-first, so a freed pid comes back at once. Real Linux hands out pids cyclically precisely to make reuse rare, and has threads, process groups, sessions, subreapers, `SIGCHLD` and `waitid` options, none of which appear here. Init is assumed to be a good citizen and never exits. Zombie children adopted by init stay zombies until something calls `pt_wait(t, 1, ...)`.

## Your turn

Implement `pt_fork`, `pt_exit`, `pt_wait` and `pt_descendants`. The hidden reference keeps its data differently (separate arrays, a transitive closure for descendants), and random schedules compare every step.
