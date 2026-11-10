---
title: Spinlocks and Interrupt State
summary: Protect a critical section with an atomic lock and restore the caller’s interrupt state.
minutes: 45
difficulty: medium
tags: kernel, systems
id: osdev-2/spinlocks
mode: harness
---
The frame bitmap, the heap's descriptor list and every wait queue are **shared state**: more than one flow of control can reach them. On a single CPU those flows are the running task and whatever interrupts it. On a multiprocessor they are also other CPUs, running kernel code at the very same moment. A **spinlock** is the kernel's basic tool for making a short stretch of code (a **critical section**) run by one flow at a time. This lesson builds one and, just as importantly, gets its interaction with interrupts right.

## Why `held = 1` is not a lock

The obvious lock is broken:

```c
while (l->held)      /* 1. wait until it is free */
    ;
l->held = 1;         /* 2. take it */
```

Two CPUs can both execute step 1, both see 0, and both execute step 2. Each believes it owns the lock. The test and the set must be **one indivisible operation**. x86 provides it as `xchg`: swap a register with memory, atomically, with the bus (or cache line) locked for the duration. GCC exposes it as a builtin:

```c
int old = __atomic_exchange_n(&l->held, 1, __ATOMIC_ACQUIRE);
/* old == 0: it was free and now it is ours.
   old == 1: someone else holds it; we wrote 1 over a 1, which changes nothing. */
```

Spinning on that exchange until it returns 0 is a correct lock. Inside the loop, the `pause` instruction tells the CPU this is a spin-wait: it saves power and avoids a costly pipeline flush when the lock is finally released.

## Ordering: acquire and release

Compilers and CPUs reorder memory accesses when they think nobody can tell. A lock is precisely the place where somebody can tell. Two guarantees are needed:

```text
lock:      exchange (ACQUIRE)  --+   no read or write of the protected data
           ... critical section ...   may move above the acquire
unlock:    store 0  (RELEASE)  --+   or below the release
```

`__ATOMIC_ACQUIRE` on the exchange stops the critical section's accesses from being hoisted before the lock is taken. `__ATOMIC_RELEASE` on the unlocking store makes every write in the critical section visible before other CPUs can see the lock as free. Unlock with `__atomic_store_n(&l->held, 0, __ATOMIC_RELEASE)`, not a plain assignment, which the compiler may move or merge.

## The single-CPU deadlock

Now the part that is specific to kernels. Suppose task code takes the lock protecting a wait queue, and while it holds it, the timer interrupt fires. The interrupt handler wants to wake a task, so it takes the same lock:

```text
task:       lock(&q_lock)          held = 1
            ... editing the queue ...
  IRQ  -->  timer handler:
            lock(&q_lock)          spins: held is 1
                                   the holder is the task we interrupted,
                                   and it cannot run until we return
            spins forever
```

Nobody else can release the lock: its owner is suspended underneath the handler on the same CPU. The machine hangs. The fix is to make sure that interrupts **cannot** arrive on this CPU while it holds a lock that an interrupt handler also takes. So the lock function disables interrupts (`cli`) **before** it acquires the lock, and the unlock function re-enables them **after** releasing it.

Note what this does and does not do. `cli` affects only the local CPU. It prevents the self-deadlock above, but another CPU can still race for the lock, so the atomic exchange is still essential. The two mechanisms solve two different problems.

Order matters in both directions. If `lock` took the lock first and then executed `cli`, an interrupt could arrive in between, inside the window the whole design exists to close. If `unlock` executed `sti` before releasing the lock, a pending interrupt would be delivered at once and its handler would spin on the still-held lock.

## Restore, don't enable

The tempting unlock ends with `sti`. That is a bug whenever the caller had interrupts **off** to begin with: an interrupt handler that takes a lock, or code that holds two locks.

```text
outer = lock_irqsave(&a)      IF was 1 -> saved, now 0
inner = lock_irqsave(&b)      IF was 0 -> saved, still 0
unlock_irqrestore(&b, inner)  must leave IF = 0: we still hold a!
unlock_irqrestore(&a, outer)  IF back to 1
```

So `lock_irqsave` returns the flags **as they were on entry** and `unlock_irqrestore` restores exactly that state. The interrupt flag, IF, is bit 9 (`0x200`) of RFLAGS. The starter's `irq_save` reads RFLAGS with `pushfq; popq` and then executes `cli`; `irq_restore` executes `sti` only if bit 9 was set in the saved value. These are the same helpers as in the preemption lesson; your job is to put them around the lock in the right order. Linux names this pair `spin_lock_irqsave` and `spin_unlock_irqrestore`.

## Rules of thumb

- Keep critical sections short. Other CPUs burn cycles spinning, and interrupts on this one are delayed.
- Never sleep while holding a spinlock. A sleeping holder means everyone else spins until it is scheduled again.
- Locks are not recursive. Taking a lock you already hold spins forever; the tests never do that.

## Your turn

`irq_save` and `irq_restore` are provided. Write `lock_irqsave` and `unlock_irqrestore`. The tests check the lock word and the real IF bit of the CPU after every step, with interrupts initially on and initially off, and with two nested locks. They never contend, so nothing depends on timing.
