static unsigned long flags_now(void) {
    unsigned long f;
    __asm__ volatile("pushfq; popq %0" : "=r"(f));
    return f;
}
static int if_on(void) { return (flags_now() & 0x200) != 0; }

void kmain(void) {
    Spinlock l = { 0 }, m = { 0 };
    unsigned long f, outer, inner;
    int mid_if, mid_held;

    bm_outb(0x21, 0xFF);                /* mask every IRQ: sti must be harmless */
    bm_outb(0xA1, 0xFF);

    /* Interrupts off on entry. */
    __asm__ volatile("cli" ::: "memory");
    f = lock_irqsave(&l);
    CHECK("lock_irqsave sets held to 1", l.held == 1, "held is %d after lock_irqsave", l.held);
    CHECK("lock_irqsave returns the entry flags (IF clear here)", (f & 0x200) == 0 && (f & 0x2) != 0,
          "returned 0x%lx; expected the real RFLAGS (bit 1 always set) with bit 9 clear", f);
    CHECK("interrupts stay disabled while the lock is held", !if_on(), "IF is set inside the critical section");
    unlock_irqrestore(&l, f);
    CHECK("unlock_irqrestore sets held back to 0", l.held == 0, "held is %d after unlock", l.held);
    CHECK("unlocking does not enable interrupts that were off before locking", !if_on(),
          "IF is set after unlock_irqrestore, but it was clear before lock_irqsave");

    /* Interrupts on at entry. */
    __asm__ volatile("sti" ::: "memory");
    f = lock_irqsave(&l);
    CHECK("lock_irqsave returns flags with IF set when interrupts were on", (f & 0x200) != 0 && (f & 0x2) != 0,
          "returned 0x%lx; expected bit 9 (0x200) set", f);
    CHECK("lock_irqsave disables interrupts", !if_on(), "IF is still set after lock_irqsave");
    CHECK("the lock is held with interrupts on at entry too", l.held == 1, "held is %d", l.held);
    unlock_irqrestore(&l, f);
    CHECK("unlock_irqrestore re-enables interrupts that were on before", if_on(), "IF is clear after unlock_irqrestore");
    CHECK("the lock is free after the second unlock", l.held == 0, "held is %d", l.held);

    /* Two different locks, nested. */
    outer = lock_irqsave(&l);
    inner = lock_irqsave(&m);
    CHECK("the inner lock saves flags with IF clear (the outer lock disabled them)", (inner & 0x200) == 0,
          "inner flags 0x%lx have IF set", inner);
    unlock_irqrestore(&m, inner);
    mid_if = if_on();
    mid_held = l.held;
    CHECK("releasing the inner lock keeps interrupts off while the outer is held", !mid_if && mid_held == 1 && m.held == 0,
          "after inner unlock: IF=%d, outer held=%d, inner held=%d", mid_if, mid_held, m.held);
    unlock_irqrestore(&l, outer);
    CHECK("releasing the outer lock restores interrupts", if_on() && l.held == 0,
          "after outer unlock: IF=%d, held=%d", if_on(), l.held);

    /* The lock can be taken again and again. */
    int ok = 1;
    for (int i = 0; i < 100; i++) {
        f = lock_irqsave(&l);
        if (l.held != 1) ok = 0;
        unlock_irqrestore(&l, f);
        if (l.held != 0 || !if_on()) ok = 0;
    }
    CHECK("100 lock/unlock cycles leave the lock free and interrupts on", ok, "a cycle left held=%d IF=%d", l.held, if_on());

    __asm__ volatile("cli" ::: "memory");
}
