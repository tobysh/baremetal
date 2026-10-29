`read_flags`: `uint64_t f; __asm__ volatile("pushfq; popq %0" : "=r"(f)); return f;`. IF is `1 << 9`. `irq_save` reads the flags first, then runs `__asm__ volatile("cli" ::: "memory")`.
===
`irq_restore(flags)` is `if (flags & (1 << 9)) __asm__ volatile("sti" ::: "memory");`. There is deliberately no `else`: if IF was clear, the interrupts stay off.
===
`atomic_cas`: use the builtin `__atomic_compare_exchange_n(p, &expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)` which returns a boolean, or write `lock cmpxchgq` yourself (expected in `rax`, so use the constraint `"+a"`). `atomic_add` is `lock xaddq` as shown in the lesson, or `__atomic_fetch_add`.
===
`spin_trylock` is `return __atomic_exchange_n(&l->locked, 1, __ATOMIC_ACQUIRE) == 0;`: if the old value was 0, you set it from free to held. `spin_lock` loops on `spin_trylock` with a `pause` in the body. `spin_unlock` is `__atomic_store_n(&l->locked, 0, __ATOMIC_RELEASE)`.
===
Order matters in the irqsave pair: `flags = irq_save(); spin_lock(l); return flags;` and `spin_unlock(l); irq_restore(flags);`. Release in the opposite order from taking. `irq_wait` is a single `__asm__ volatile("sti; hlt" ::: "memory");`.
