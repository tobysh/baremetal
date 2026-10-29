---
title: Signals and Ctrl-C
summary: Make the shell survive SIGINT and SIGQUIT while the programs it starts still die from them, using sigaction and a reset before exec.
minutes: 35
difficulty: hard
tags: signals, processes, shell
id: c-shell/signals
mode: harness
---
Press Ctrl-C while `sleep 100` runs in your shell. The sleep dies, the shell does not. The terminal sends **SIGINT** to everything in the foreground, shell included, so the shell must have arranged to ignore or handle it, while the child must still be killable.

## sigaction

`signal()` has murky semantics; use `sigaction`:

```c
struct sigaction sa = {0};
sa.sa_handler = on_sigint;        // or SIG_IGN, or SIG_DFL
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_RESTART;         // restart interrupted slow syscalls
sigaction(SIGINT, &sa, NULL);
```

A **handler** runs asynchronously, between any two instructions of your program, so it must do almost nothing. The one safe pattern is to set a flag of type `volatile sig_atomic_t` and let the main loop notice:

```c
volatile sig_atomic_t sigint_seen;
static void on_sigint(int sig) { (void)sig; sigint_seen = 1; }
```

Never call `printf` or `malloc` in a handler.

## What exec does to signals

`fork` copies all signal settings. `exec` then resets every signal that has a **handler** back to the default (the handler's code is gone, after all), but signals set to **ignore** stay ignored. This has a nasty consequence: if the shell ignores `SIGQUIT`, every program it starts also ignores it, unless the child puts it back to `SIG_DFL` before `exec`. A shell that forgets this makes Ctrl-\ useless in every program it launches.

## Who dies of what

A child killed by signal *n* is reported by `waitpid` as signalled, which a shell shows as status 128 + *n*: 130 for SIGINT, 131 for SIGQUIT.

## Your turn

Write the shell side: install the handlers, and run a foreground command whose child restores default behaviour. The starter defines `sigint_seen`; the tests read it.
