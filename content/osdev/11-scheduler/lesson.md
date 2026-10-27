---
title: Deciding Who Runs: A Scheduler
summary: Decide which task runs next. Build a round-robin scheduler with ready, running, sleeping, blocked and dead states and an idle task for when nothing can run.
minutes: 45
difficulty: medium
tags: scheduling, processes
id: osdev/scheduler
mode: harness
---
A CPU core runs one thing at a time, yet your computer seems to do dozens at once. The trick is **multitasking**: the kernel switches between **tasks** so quickly that they appear to run in parallel. Two separate pieces make that work:

- The **scheduler** decides *which* task should run next. That's this lesson, and it's ordinary data-structure code.
- The **context switch** actually moves the CPU from one task to another. That's the next lesson, and it's assembly.

Keeping them apart is a good habit: you can test the scheduler's decisions without moving a single register.

## Task states

Every task is in exactly one state:

```
              sched_add
                 |
                 v
   +-------> READY <---------------------+
   |           |                         |
   | (preempted)| sched_pick_next        | sched_tick (time is up)
   |           v                         | sched_unblock
   +------- RUNNING --sched_sleep--> SLEEPING
               |  \
               |   +---sched_block----> BLOCKED
               |
               +-----sched_exit-----> DEAD
```

- **READY**: could run, waiting for its turn.
- **RUNNING**: on the CPU right now. Exactly one task is RUNNING.
- **SLEEPING**: waits for a time, recorded as `wake_tick`.
- **BLOCKED**: waits for an event (a key press, a lock). Someone must `sched_unblock` it.
- **DEAD**: finished. Its slot can be reused.

## Round robin

The simplest fair policy: keep the tasks in a fixed order and give each its turn in rotation. The scheduler starts looking at the slot *after* the current task, wraps around the end, and picks the first READY one.

When no task is READY, the CPU still has to run something. The **idle task** is a task that is always runnable and does nothing useful (in a real kernel it executes `hlt` in a loop). The scheduler returns idle when there is nothing else. The idle task is not in the table of ordinary tasks.

## Time

The timer interrupt ([lesson 6](/learn/osdev/pic-timer)) calls `sched_tick` once per tick. It advances the clock and moves every SLEEPING task whose `wake_tick` has arrived back to READY.

## A worked trace

Three tasks A, B, C are added in that order, so they occupy slots 0, 1, 2. The idle task is running. Here is every step, with the state of each task after it. (`R` = ready, `RUN` = running, `S` = sleeping, `-` = nothing changed.)

| Step | Call | A | B | C | Current |
|------|------|---|---|---|---------|
| 1 | `sched_pick_next()` | RUN | R | R | A (search began at slot 0) |
| 2 | `sched_sleep(3)` | S (wake at 3) | R | R | A |
| 3 | `sched_pick_next()` | S | RUN | R | B (A isn't running, so it stays asleep) |
| 4 | `sched_pick_next()` | S | R | RUN | C |
| 5 | `sched_tick()` (now = 1) | S | R | RUN | C |
| 6 | `sched_pick_next()` | S | RUN | R | B (the search from slot 2 wraps to slot 0, skips A, finds B) |
| 7 | `sched_tick()` ×2 (now = 3) | R | RUN | R | B (A wakes: `wake_tick <= now`) |
| 8 | `sched_pick_next()` | R | R | RUN | C (search starts after B's slot) |
| 9 | `sched_pick_next()` | RUN | R | R | A |

Notice step 8: A has been ready since step 7, but it doesn't jump the queue. It takes its turn in the rotation. That is what "round robin" means, and it is why no task can be starved by an eager neighbour.

## Why this policy

Round robin is not the best scheduler, only the simplest *fair* one. Real systems add **priorities** (interactive tasks should run before a background compile), **time slices** (how many ticks a task may run before being preempted) and **multiple queues** per CPU. Linux's current scheduler keeps a tree ordered by how much CPU time each task has already received and always runs the one that has received the least. The shape of your code (state per task, a `pick_next` that returns a decision, a `tick` that moves time forward) is the same core idea.

## Cooperative and preemptive

Your scheduler only *chooses*. Whether a task gets interrupted depends on who calls `sched_pick_next`:

- **Cooperative:** a task calls it voluntarily (by yielding, sleeping or blocking). One task that never yields hogs the CPU.
- **Preemptive:** the timer interrupt from [lesson 6](/learn/osdev/pic-timer) calls it every few ticks, so a task that loops forever still loses the CPU. The [next lesson](/learn/osdev/context-switch) supplies the switching, and putting the timer handler, scheduler and switch together gives you a preemptive kernel.

## Debugging checklist

- **The same task runs forever:** you didn't move `RUNNING` back to `READY` before searching, or `current_slot` isn't updated when you pick.
- **A task is skipped once per rotation:** the search starts at the current slot instead of after it, or doesn't cover the current task's own slot last.
- **Dead tasks run again:** `sched_add` reuses a dead slot, but the search must treat only `READY` as eligible.
- **Idle never returns:** the idle task sits outside the table, so don't try to find it by scanning slots; return the saved pointer.
- **A sleeper never wakes:** compare with `<=`, not `==`. A tick could be missed (or counted twice) and equality would never match again.

## Your turn

The scheduler functions only *decide and record*. They don't switch anything, so a test (or the real timer interrupt handler) calls `sched_pick_next` and gets a pointer to what should run. The rules are in the task description: read them carefully, because the tests check the exact order.
