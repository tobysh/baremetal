---
title: Sleep and Wake Queues
summary: Block tasks on a wait queue and wake them in FIFO order.
minutes: 45
difficulty: medium
tags: kernel, systems
id: osdev-2/sleep-wake
mode: harness
---
With [preemption](/learn/osdev-2/preemption) every task gets the CPU in turn. But most tasks, most of the time, have nothing to do: a shell waits for a key, a reader waits for a pipe to fill, a driver waits for the disk. A task that waits by **polling**

```c
while (!data_ready)
    ;                 /* burns its whole time slice checking a flag */
```

wastes every tick it is given, and slows down the tasks that do have work. The kernel's answer is to take a waiting task **off the CPU entirely** until the event happens. The data structure that makes this possible is the **wait queue**: a list of tasks blocked on one particular event. Every blocking operation in a kernel (reading a pipe, waiting for a child, `sleep()`, acquiring a mutex) is a wait queue underneath.

## The task states

The scheduler from the preemption lesson picks only tasks in the `READY` state. Blocking is just a state change that makes the scheduler skip the task:

```text
           sleep_on(q, t)
  READY  ----------------->  BLOCKED      (t is now on q, the scheduler ignores it)
         <-----------------
           wake_one(q)                    (t is off q, the scheduler may pick it again)
```

A blocked task's registers are saved on its own stack by the context switch, exactly as for a preempted task. Nothing is lost while it sleeps; it simply is not chosen. When the event happens, somebody (an interrupt handler, another task) calls `wake_one` on the queue. The woken task goes back to `READY` and resumes from inside `sleep_on` the next time the scheduler picks it.

One event, one queue. A kernel has many queues (one per pipe, one per child-exit, one per lock), and a blocked task is on **exactly one** of them. That is why `sleep_on` refuses a task that is not `READY`: a task that is already blocked is already on some queue, and putting it on a second one would let two different events wake it, one of which would then find a task that is no longer there.

## FIFO order and the ring buffer

Wake order matters for fairness. If waiters were woken last-in-first-out, a busy system could keep waking the newest waiter and starve the oldest forever. A wait queue is therefore **first in, first out**.

A FIFO with a fixed capacity is a **ring buffer**: an array, a `head` index (the oldest entry) and a `count`. New entries go at `(head + count) % CAP`; the oldest is removed from `head`, which then advances. Neither operation shifts any elements.

```text
CAP = 8.  Waiters T0..T4 slept in order; T0..T2 were then woken.

index:   0    1    2    3    4    5    6    7
       [ .  | .  | .  | T3 | T4 | .  | .  | .  ]
                        ^head=3   count=2

sleep_on(T5..T9):  slots 5, 6, 7, then (3 + 5) % 8 = 0, 1
       [ T8 | T9 | .  | T3 | T4 | T5 | T6 | T7 ]
                        ^head=3   count=7
wake_one -> T3, head = 4
```

Storing `count` rather than a separate tail index avoids the classic ambiguity of ring buffers: with only `head` and `tail`, "empty" and "full" both look like `head == tail`. With `count`, empty is `count == 0` and full is `count == CAP`.

## Rules for the operations

`sleep_on(q, t)` enqueues `t` at the tail and sets it `BLOCKED`. It refuses with -1 if the task is not `READY` or if the queue is full, and in both cases changes **nothing**: neither the task's state nor the queue. A half-done enqueue (state changed, task not stored) leaves a task that is blocked forever with nobody to wake it.

`wake_one(q)` removes the task at the head, sets it `READY` and returns it. Waking an empty queue is not an error: it returns `NULL`. Events often fire when nobody happens to be waiting (a key press with no reader), and the waker should not have to check first.

`wait_init(q)` makes a queue empty. Queues are often embedded in larger structures that start out as uninitialised memory, so `wait_init` must not depend on what was there before.

## The lost-wakeup race

In a real kernel the sleeper does something like this:

```c
while (!pipe_has_data(p))
    sleep_on(&p->readers, current);    /* plus a switch to another task */
```

Consider this interleaving on one CPU:

```text
reader                                writer (or an interrupt)
------                                ------
checks pipe: empty
                                      puts a byte in the pipe
                                      wake_one(&p->readers) -> NULL, nobody there
sleep_on(&p->readers, me)
... sleeps forever, the byte is already there
```

The wakeup happened *between* the check and the sleep, so it was lost. The cure is that the check and the enqueue happen under the **same lock** the waker takes, so the waker cannot run in the gap. In this exercise the caller holds that lock; you write the queue operations it calls. The scheduler then releases the lock *as part of* switching away, which is why kernel sleep functions usually take the lock as an argument.

## Your turn

The `Task` and `WaitQueue` types are in the starter. Implement the three operations as a ring buffer of capacity 8. The tests fill queues, wrap them around, reject invalid sleeps and use two queues at once.
