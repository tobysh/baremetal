Two separate problems are being solved. The atomic exchange keeps *other CPUs* out of the critical section. Disabling interrupts keeps *this CPU's own interrupt handlers* out, because a handler that spins on a lock held by the code it interrupted can never make progress. You need both, in a particular order.
===
`lock_irqsave` starts with `irq_save()` and keeps its return value; that value is what you return at the end. Only after interrupts are off do you try to take the lock. `unlock_irqrestore` is the mirror image: release first, then `irq_restore(flags)`. Never write `sti` directly: the caller may have had interrupts off before it locked.
===
The exchange loop: `__atomic_exchange_n(&l->held, 1, __ATOMIC_ACQUIRE)` returns the *old* value. Keep trying while it returns non-zero. For the unlock, a plain `l->held = 0` is not enough ordering; use `__atomic_store_n` with `__ATOMIC_RELEASE`.
===
The same save/restore shape around a different resource, a counter shared with an interrupt handler:

```c
unsigned long f = irq_save();
pending_events++;          /* the handler cannot run in between */
irq_restore(f);            /* back to whatever the caller had */
```
