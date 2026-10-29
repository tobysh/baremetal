---
title: Critical Sections: Interrupts, Atomics and Spinlocks
summary: Save and restore the interrupt flag, build atomic add and compare-and-swap, and write a spinlock that is safe against interrupt handlers.
minutes: 50
difficulty: medium
tags: concurrency, interrupts, atomics
id: osdev/irq-locks
mode: harness
---
You now have code that runs at two levels: your normal kernel code, and **interrupt handlers** that can barge in between any two instructions. If both touch the same data, you get a **race condition**. For example:

```c
// main code                      // timer handler
queue_len = queue_len + 1;        queue_len = queue_len - 1;
```

`queue_len = queue_len + 1` is really three instructions: load, add, store. If the timer fires between the load and the store, the handler's decrement is overwritten. The data is silently wrong, and only once in a million runs. Kernels exist in a permanent fight against this, and the tools are small.

## Tool 1: turn interrupts off for a moment

The CPU's **interrupt flag** (IF, bit 9 of `RFLAGS`) decides whether it accepts maskable interrupts. `cli` clears it and `sti` sets it. The tempting pattern is:

```c
cli();
/* touch shared data */
sti();
```

but it has a bug: if interrupts were *already off* when the function was called, `sti` turns them on too early. The correct pattern **saves** the old state and puts back exactly that:

```c
uint64_t flags = irq_save();   // reads RFLAGS, then cli
/* touch shared data */
irq_restore(flags);            // sti only if IF was set in flags
```

Now these calls nest correctly: the inner restore leaves interrupts off because the outer call had turned them off.

You read `RFLAGS` with `pushfq; pop %rax`. Do it in inline assembly, with a `"memory"` clobber on `cli` and `sti`, so that the compiler doesn't move loads and stores of shared data out of the protected region.

## Tool 2: atomic instructions

Sometimes you don't need to stop the world; one instruction that cannot be interrupted halfway is enough. On x86 a read-modify-write instruction with the `lock` prefix is **atomic**, even with several CPUs:

- `lock xadd` adds a value to memory and returns the old value.
- `lock cmpxchg` (**compare-and-swap**) stores a new value only if memory still holds the expected one, and tells you whether it did.
- `xchg` with a memory operand is always locked and swaps a register with memory.

```c
// returns the value before the addition
uint64_t atomic_add(volatile uint64_t *p, uint64_t v) {
    __asm__ volatile("lock xaddq %0, %1" : "+r"(v), "+m"(*p) : : "memory");
    return v;
}
```

## Tool 3: the spinlock

A **spinlock** is one word: 0 means free, 1 means held. To take it, atomically swap a 1 into it; if the old value was 0 you got the lock, otherwise somebody else has it and you try again (**spin**). To release, store 0.

```c
while (__atomic_exchange_n(&lock->locked, 1, __ATOMIC_ACQUIRE)) {
    __asm__ volatile("pause");     // be polite to the other CPU
}
```

On a single CPU a plain spinlock has a famous deadlock. Suppose the main code holds the lock and an interrupt fires. The handler tries to take the same lock and spins forever, waiting for code that cannot run until the handler returns. The cure is to disable interrupts **before** taking the lock whenever any handler might want it: `spin_lock_irqsave`.

## Tool 4: sleeping until something happens

An idle kernel should not spin; it should `hlt`, which stops the CPU until an interrupt arrives. But this is a trap:

```c
if (nothing_to_do) hlt;    // BUG
```

If the interrupt arrives between the `if` and the `hlt`, the CPU halts anyway and sleeps through the thing it was waiting for. The fix is that `sti` takes effect only after the *next* instruction, so `sti; hlt` as a pair cannot be interrupted in between. Check the condition with interrupts off, then use `sti; hlt` to wait.

## Debugging checklist

- **Interrupts get turned on by accident:** `irq_restore` must test the saved IF bit; never call `sti` unconditionally.
- **Atomic add returns the new value:** `xadd` leaves the *old* value in the register.
- **The lock works in tests and fails in a loop:** is the lock word `volatile`, and are the barriers (`"memory"` clobbers or `__atomic` builtins) in place?
- **The kernel locks up in the timer handler:** the lock holder didn't disable interrupts.

## Your turn

The tests start a real 1000 Hz timer. They hold interrupts off with your functions and check that not a single tick gets through, then release them and check the ticks return.
