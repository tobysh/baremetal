---
title: Deadlock Snapshot
summary: A hung program leaves a table of who holds and who waits. Build the wait-for graph, find the cycle, and name the lock order that would have prevented it.
minutes: 30
difficulty: medium
tags: debugging, concurrency, deadlock, evidence
id: c-debugging/deadlock-snapshot
mode: harness
---
A hung multi-threaded program shows no error and no crash. What you can still get is a **snapshot**: for every thread, which locks it holds and which lock it is blocked on. This lesson works only with that table, as plain data. No threads are started.

## The evidence

```text
thread  holds      waits for
  1     A          B
  2     B          C
  3     C          A
  4     D          -
```

Read it row by row. Thread 1 holds `A` and wants `B`. `B` belongs to thread 2, so thread 1 cannot move until thread 2 lets go. Thread 2 wants `C`, which thread 3 holds. Thread 3 wants `A`, which thread 1 holds. Nobody can ever proceed. Thread 4 holds a lock but is not waiting, so it is not part of the problem.

## The wait-for graph

Draw an arrow from each waiting thread to the thread that holds the lock it wants:

```text
1 -> 2 -> 3 -> 1        (4 has no arrow)
```

Each thread waits for at most one lock, so each thread has **at most one outgoing arrow**. That makes the graph easy to walk: start anywhere and keep following arrows.

- If you reach a thread with no arrow, the chain ends. That chain is only a *queue of waiters*; the thread at the end is running or waiting for something that is not locked. It is **not** a deadlock.
- If you come back to a thread you have already visited, you found a **cycle**. Every thread on a cycle is deadlocked, and so is every thread whose chain leads into it, but only the cycle itself is the *cause*.

A thread waiting on a lock that nobody holds is a different bug (a lost wake-up or a lock that was never taken). The graph has no arrow for it, and it is not a cycle.

A thread that waits for a lock it already holds forms a cycle of length 1 (a non-recursive mutex locked twice).

## Reading the cycle

Once you have the cycle, the locks it waits on are the *conflict*. In the example, the threads took `A`, `B`, `C` in the orders `A then B`, `B then C` and `C then A`. No single global order satisfies all three. The standard cure is a **lock ordering**: give every lock a rank and always acquire in ascending rank. Then a thread that holds a higher lock never waits for a lower one, and a cycle cannot form.

## Your turn

The snapshot is an array of `thread_state`. Thread ids and lock ids are arbitrary integers (not array positions). Implement:

- `find_deadlock`: report the cycle.
- `suggest_order`: report the locks involved in the cycle in the order they should be taken.
