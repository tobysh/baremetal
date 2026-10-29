---
title: Deadlock and Lock Ordering
summary: Two threads that lock two mutexes in opposite orders can freeze forever. Fix it by always locking in one agreed order.
minutes: 25
difficulty: medium
tags: threads, mutex, deadlock
id: c-concurrency/lock-ordering
mode: harness
---
One mutex is easy. Two mutexes introduce a new way to fail: **deadlock**.

Imagine a bank transfer that locks both accounts:

```c
lock(from);
lock(to);
// move the money
unlock(to);
unlock(from);
```

Thread A moves money from account 1 to account 2, so it locks 1 and then wants 2. At the same moment thread B moves from 2 to 1, so it locks 2 and then wants 1. Each holds the lock the other one needs, and both wait forever. The program just hangs, with no error message.

## The four ingredients

Deadlock needs all of these at once: locks can be held by only one thread, a thread holds one lock while waiting for another, nobody takes a lock away by force, and the waiting forms a circle. Remove any one and it cannot happen. The cheapest one to remove is the circle.

## Lock ordering

Give every lock a **rank**, and always take locks in rank order. If every thread locks the lower rank first, a circle is impossible: a thread that holds a high-ranked lock never waits for a lower one.

For accounts, a natural rank is the account's `id`:

```c
Account *first  = from->id < to->id ? from : to;
Account *second = from->id < to->id ? to : from;
lock(first);
lock(second);
```

Note the order you **lock** in is by rank, but the money still flows from `from` to `to`.

Other things to watch for:

- Locking the same non-recursive mutex twice in one thread deadlocks with itself. A transfer from an account to itself must be handled before you lock anything.
- Unlock on every path, including the "not enough money" path.

## Your turn

The starter transfers money but locks in whatever order it was called. It works in a single thread and then freezes under load. The test has a watchdog that reports a freeze instead of waiting forever. Fix `transfer`.
