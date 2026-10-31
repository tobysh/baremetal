---
title: Signals, Masks and Delivery
summary: Model pending and blocked sets, dispositions, coalescing, stop and continue, and why SIGKILL and SIGSTOP cannot be blocked, caught or ignored.
minutes: 40
difficulty: hard
tags: signals, kernel-model, state-machine
id: c-linux-internals/signal-masks
mode: harness
---
A signal is a notification delivered to a process. A process has two 32-bit sets and a table:

- **pending**: signals generated but not yet delivered (bit `n` is signal `n`),
- **blocked**: the signal mask. A blocked signal stays pending,
- **dispositions**: for each signal, default action, ignore, or catch (run a handler).

## Rules this model implements

- **Standard signals coalesce.** Sending `SIGUSR1` three times while it is blocked leaves one bit set, and one delivery.
- **Delivery order:** the lowest-numbered signal that is pending and not blocked.
- **SIGKILL and SIGSTOP** cannot be blocked, caught or ignored. `sig_mask` silently drops them from any mask you give it, and `sig_set_disp` refuses them.
- **Ignored signals are discarded at generation**, unless they are blocked, in which case they stay pending and are dropped when delivered. Setting a disposition that ignores a signal also discards a pending instance. `SIGCHLD` and `SIGCONT` are ignored by default.
- **Stop and continue.** A stopping signal (`SIGSTOP`, `SIGTSTP`) with the default action stops the process, which then receives nothing but `SIGKILL`. `SIGCONT` resumes it **at the moment it is sent**, whatever the mask or disposition, and discards pending stop signals; sending a stop signal discards a pending `SIGCONT`.
- **Default termination:** the process dies with `term_sig` set and its pending set cleared.

The given helpers `default_action` and `is_ignored` encode the defaults. Signals 1 to 31 are valid, and the numbers are the x86 Linux ones.

## Model limits

Handlers do not run code here: delivery appends the signal number to `handled[]`. Real kernels also block a signal while its own handler runs, honour `sa_mask` and `SA_RESTART`, queue real-time signals, have per-thread masks, and interrupt system calls with `EINTR`. None of that is modelled, and we only keep 32 log entries.

## Your turn

Implement `sig_set_disp`, `sig_mask`, `sig_send` and `sig_deliver`. The public example shows two sends collapsing into one delivery. The hidden tests compare thousands of random schedules with a reference that keeps one flag per signal.
