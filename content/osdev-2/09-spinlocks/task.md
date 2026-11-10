The `Spinlock` type (one `int held`, 0 when free) and the helpers `irq_save()` (returns RFLAGS, then disables interrupts) and `irq_restore(flags)` (enables interrupts only if bit 9 of `flags` is set) are in the starter. Implement:

- `unsigned long lock_irqsave(Spinlock *l)` disables interrupts with `irq_save` first, then spins until an atomic exchange of `l->held` with 1 returns 0 (use acquire ordering and `pause` in the loop). It returns the flags `irq_save` returned, so the caller can restore them.
- `void unlock_irqrestore(Spinlock *l, unsigned long flags)` stores 0 in `l->held` with release ordering, then calls `irq_restore(flags)`.

After `unlock_irqrestore`, interrupts must be enabled exactly if they were enabled before the matching `lock_irqsave`. Locks may nest when they are different locks; the same lock is never taken twice. The tests never contend for a lock.
