Implement the following. `SpinLock` is declared in the starter.

- `uint64_t read_flags(void)` returns `RFLAGS`. `int irq_enabled(void)` returns 1 if the interrupt flag (bit 9) is set.
- `uint64_t irq_save(void)` returns the current `RFLAGS` and then disables interrupts. `void irq_restore(uint64_t flags)` enables interrupts only if the IF bit is set in `flags`, and otherwise changes nothing.
- `uint64_t atomic_add(volatile uint64_t *p, uint64_t v)` atomically adds `v` to `*p` and returns the value `*p` had before. `int atomic_cas(volatile uint64_t *p, uint64_t expected, uint64_t desired)` stores `desired` only if `*p == expected` and returns 1 if it did, else 0.
- `void spin_init(SpinLock *l)` makes the lock free. `int spin_trylock(SpinLock *l)` takes it if it is free and returns 1, otherwise returns 0 without waiting. `void spin_lock(SpinLock *l)` spins until it holds the lock. `void spin_unlock(SpinLock *l)` releases it.
- `uint64_t spin_lock_irqsave(SpinLock *l)` disables interrupts (saving the old state), then takes the lock, and returns the saved flags. `void spin_unlock_irqrestore(SpinLock *l, uint64_t flags)` releases the lock, then restores the flags.
- `void irq_wait(void)` enables interrupts and halts the CPU until the next interrupt, with `sti` immediately followed by `hlt`. It returns with interrupts enabled.
