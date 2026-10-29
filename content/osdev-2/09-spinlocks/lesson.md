---
title: Spinlocks and Interrupt State
summary: Protect a critical section with an atomic lock and restore the caller’s interrupt state.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/spinlocks
mode: harness
---
A preemptive kernel can interrupt itself while it holds a lock. If the interrupt handler tries to acquire that same lock on the same CPU, spinning cannot make progress. Disable local interrupts before acquiring such a lock and restore the previous interrupt-enable state after releasing it. Disabling interrupts alone does not exclude other CPUs; the lock still needs an atomic exchange.

The helpers supplied here save RFLAGS and execute `cli`, then restore IF only if it was originally set. `lock_irqsave` returns the saved flags. This lesson tests real IF state as well as lock state; its uncontended calls avoid relying on timing.
