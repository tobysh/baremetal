---
title: Preemption: Switching Tasks from the Timer
summary: Use timer interrupts to preempt tasks and schedule them round-robin.
minutes: 75
difficulty: hard
tags: scheduling, interrupts, processes
id: osdev-2/preemption
mode: harness
---
In Part 1 you built a [scheduler](/learn/osdev/scheduler) that only moved data around, and a [context switch](/learn/osdev/context-switch) that tasks called *themselves*. That is **cooperative** multitasking: it works only while every task is polite enough to call `yield`. One task stuck in `for (;;) {}` and the machine is dead.

A real kernel uses **preemption**. The hardware timer interrupts the running task whether it likes it or not, and the interrupt handler may decide that somebody else should run next. This lesson joins the pieces you already have: the PIT tick, `ctx_switch` and a round-robin pick.

## Switching inside an interrupt

Think about what the stack looks like when the timer fires while task A runs:

```
A's stack:   ... A's own frames ...
             [ rip rflags cs rsp ss ]      pushed by the CPU
             [ all registers ]             pushed by the boot stub
             [ bm_dispatch -> irq_dispatch -> timer_tick -> schedule -> ctx_switch ]
```

At that point A's *entire* state, including the interrupted instruction, is saved on A's own stack. If `ctx_switch` now moves to task B's stack, B resumes wherever it stopped, and when it eventually returns out of its own `timer_tick` call, the stub pops *its* registers and `iretq`s into B's code. Nothing special is needed: **each task carries its own interrupt frame on its own stack**. This is why the context switch you wrote last time is all the hardware-facing code a preemptive scheduler needs.

Three details make or break it:

1. **Acknowledge the interrupt before you switch.** The PIC will not deliver another timer tick until it gets an EOI. If you switch first, the new task might run for a long time (or forever) before the EOI is sent, and the system stalls with no more ticks. So `timer_tick` sends the EOI first.
2. **New tasks start with interrupts off.** A task that has never run is "resumed" by `ctx_switch` from inside someone's `schedule()` call, and interrupts are disabled there. Its first instructions are in `task_trampoline` (provided), which executes `sti` before calling the task's entry function.
3. **Protect `schedule` from itself.** If a tick arrived while `yield` was in the middle of choosing the next task, the handler would run `schedule` again and corrupt the scheduler's state. Code that touches scheduler data must run with interrupts off. But it must put interrupts back **exactly as they were**: if a caller already had them off, enabling them on the way out would be a bug. That is the job of `irq_save` and `irq_restore`:

```c
unsigned long flags = irq_save();   // remember IF, then cli
/* ... touch shared state ... */
irq_restore(flags);                 // sti only if IF was set before
```

`irq_save` reads `rflags` with `pushfq`, pops it into a variable and executes `cli`. Bit 9 (`0x200`) of `rflags` is the interrupt flag. Because the save and restore are paired, they nest correctly.

## Round robin

`pick_next` scans the task table starting at the slot **after** the current one, wrapping around once (the current task's own slot is the last one checked), and returns the first task whose state is `T_READY`. If there is none it returns the boot task in slot 0. The boot task never blocks in this lesson, so there is always somebody to run.

`schedule()` does the bookkeeping: a task that was `T_RUNNING` becomes `T_READY`, the next one becomes `T_RUNNING` with a fresh time slice, `current` and the `switches` counter are updated and `ctx_switch` runs. The caller must have interrupts off. If the pick is the task already running, do nothing more.

`timer_tick` runs once per timer interrupt: count the tick, send the EOI, and subtract one from the current task's `slice`. When the slice reaches zero, call `schedule()`. Each task therefore gets `QUANTUM` (5) ticks at a time.

`yield()` is the polite version: `schedule()` with interrupts off, then restore them. `task_exit()` marks the current task `T_DEAD` and schedules away for good; the slot can be reused by `task_create` later.

## Your turn

The task table, `ctx_switch`, `task_create`, `task_trampoline` and `irq_start(hz)` (IDT, PIC, PIT and `sti`) are in the starter. You write the seven functions in the task description. The tests spin three tasks that never yield and check that all of them make progress.
