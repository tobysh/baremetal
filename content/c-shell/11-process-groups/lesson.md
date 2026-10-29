---
title: Process Groups and Job Control
summary: Put each job in its own process group, signal the whole group at once, and notice when a job is stopped with waitpid and WUNTRACED.
minutes: 40
difficulty: hard
tags: signals, processes, shell
id: c-shell/process-groups
mode: harness
---
A pipeline such as `sleep 100 | sleep 100 | sleep 100` is three processes, but you think of it as one **job**. When you press Ctrl-C or Ctrl-Z, all three must react together. Unix gives you the tool for this: the **process group**.

## Groups

Every process belongs to one process group, identified by a number (the pid of the group's first member, the *leader*). Children start in their parent's group. Two calls matter:

```c
setpgid(pid, pgid);   // move process pid into group pgid
getpgid(pid);         // which group is pid in?
setpgid(0, 0);        // "make me the leader of a new group named after my own pid"
```

A signal can be sent to a whole group by giving `kill` a **negative** pid:

```c
kill(-pgid, SIGTERM);   // every member of the group
killpg(pgid, SIGTERM);  // the same thing
```

## Doing it without a race

Shells call `setpgid` in **both** the child and the parent. Whichever runs first wins, and both calls do the same thing, so it is safe. If you only did it in the child, the parent might run `kill(-pgid, ...)` before the child has moved, and the group would not exist yet. If you only did it in the parent, the child might `exec` first. After `exec` the parent is no longer allowed to move it, and `setpgid` fails with `EACCES` (ignore that error in the parent).

A real interactive shell also hands the terminal to the job's group with `tcsetpgrp`, so the terminal sends Ctrl-C only to the foreground job. This sandbox has no terminal, so here you will use `kill` directly.

## Stopped jobs

SIGSTOP and SIGTSTP (Ctrl-Z) **stop** a process; SIGCONT resumes it. A plain `waitpid` does not report a stop. Add the flag `WUNTRACED`:

```c
waitpid(pid, &status, WUNTRACED);
if (WIFSTOPPED(status)) { /* the job was stopped; WSTOPSIG(status) says by which signal */ }
```

This is how `fg`, `bg` and `jobs` know the state of each job.

## Your turn

Write three small helpers.
