---
title: Sleep and Wake Queues
summary: Block tasks on a wait queue and wake them in FIFO order.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/sleep-wake
mode: harness
---
A task that polls for an event wastes CPU time. A wait queue records blocked tasks until an event makes them runnable. Building on [preemption](/learn/osdev-2/preemption), this exercise isolates the state transitions from context switching: the caller holds the scheduler lock while adding and removing waiters. Without that shared lock, checking a condition and sleeping can race with the wakeup and lose it.

Each task can be on at most one queue. Queue operations preserve FIFO order, and waking an empty queue is harmless. A real scheduler must atomically release its lock as it switches away and reacquire it when the task resumes.
