extern void *bm_isr_table[48];
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

typedef struct __attribute__((packed)) { uint16_t lo, sel; uint8_t ist, type; uint16_t mid; uint32_t hi, zero; } TGate;
static TGate t_idt[48];
static volatile uint64_t t_ticks;

static void t_hook(uint64_t vec, uint64_t err, uint64_t rip) {
    (void)err; (void)rip;
    if (vec == 32) { t_ticks++; bm_outb(0x20, 0x20); }
}

static uint64_t t_rdtsc(void) { uint32_t lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }
static void t_cli(void) { __asm__ volatile("cli" ::: "memory"); }
static void t_sti(void) { __asm__ volatile("sti" ::: "memory"); }

static void t_timer_on(void) {
    for (int i = 0; i < 48; i++) {
        uint64_t a = (uint64_t)(uintptr_t)bm_isr_table[i];
        t_idt[i] = (TGate){(uint16_t)a, 0x08, 0, 0x8E, (uint16_t)(a >> 16), (uint32_t)(a >> 32), 0};
    }
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } p = {sizeof t_idt - 1, (uint64_t)(uintptr_t)t_idt};
    __asm__ volatile("lidt %0" : : "m"(p));
    bm_outb(0x20, 0x11); bm_outb(0xA0, 0x11);
    bm_outb(0x21, 32); bm_outb(0xA1, 40);
    bm_outb(0x21, 4); bm_outb(0xA1, 2);
    bm_outb(0x21, 1); bm_outb(0xA1, 1);
    bm_outb(0x21, 0xFE); bm_outb(0xA1, 0xFF);
    bm_outb(0x43, 0x34);
    bm_outb(0x40, 1193 & 0xFF); bm_outb(0x40, 1193 >> 8);   // about 1000 Hz
    bm_exception_hook = t_hook;
}

static int t_wait_ticks(uint64_t target) {
    for (unsigned long spins = 0; t_ticks < target; spins++) {
        if (spins > 60000000UL) return 0;
        __asm__ volatile("pause");
    }
    return 1;
}

// Spins about `cycles` TSC units without touching the tick counter.
static void t_burn(uint64_t cycles) {
    uint64_t start = t_rdtsc();
    while (t_rdtsc() - start < cycles) __asm__ volatile("pause");
}

void kmain(void) {
    t_cli();
    CHECK("irq_enabled is 0 after cli", irq_enabled() == 0, "returned %d", irq_enabled());
    t_sti();
    CHECK("irq_enabled is 1 after sti", irq_enabled() == 1, "returned %d", irq_enabled());
    t_cli();
    CHECK("read_flags returns the real RFLAGS (bit 1 is always set, IF is clear)", (read_flags() & 2) != 0 && (read_flags() & (1 << 9)) == 0, "RFLAGS is 0x%lx", read_flags());

    uint64_t f1 = irq_save();
    CHECK("irq_save with interrupts off reports them off", (f1 & (1 << 9)) == 0 && !irq_enabled(), "flags 0x%lx", f1);
    irq_restore(f1);
    CHECK("irq_restore leaves interrupts off if they were off", !irq_enabled(), "interrupts were turned on");

    t_sti();
    uint64_t f2 = irq_save();
    CHECK("irq_save with interrupts on reports IF set", (f2 & (1 << 9)) != 0, "flags 0x%lx", f2);
    CHECK("irq_save disables interrupts", !irq_enabled(), "interrupts are still enabled");
    irq_restore(f2);
    CHECK("irq_restore re-enables interrupts that were on", irq_enabled(), "interrupts are still disabled");

    uint64_t outer = irq_save();
    uint64_t inner = irq_save();
    irq_restore(inner);
    int still_off = !irq_enabled();
    irq_restore(outer);
    CHECK("nested irq_save/irq_restore only re-enables at the outermost level", still_off && irq_enabled(), "inner restore left IF %s, outer left IF %s", still_off ? "off" : "on", irq_enabled() ? "on" : "off");
    t_cli();

    volatile uint64_t v = 40;
    uint64_t old = atomic_add(&v, 2);
    CHECK("atomic_add returns the old value and adds", old == 40 && v == 42, "returned %lu, value is %lu", old, v);
    v = 0xFFFFFFFFFFFFFFFFULL;
    old = atomic_add(&v, 1);
    CHECK("atomic_add wraps around at 64 bits", old == 0xFFFFFFFFFFFFFFFFULL && v == 0, "returned 0x%lx, value is 0x%lx", old, v);
    v = 5;
    CHECK("atomic_cas succeeds when the value matches", atomic_cas(&v, 5, 9) == 1 && v == 9, "value is %lu", v);
    CHECK("atomic_cas fails and changes nothing when it differs", atomic_cas(&v, 5, 77) == 0 && v == 9, "value is %lu", v);

    SpinLock lk;
    lk.locked = 0xDEAD;
    spin_init(&lk);
    CHECK("spin_init makes the lock free", lk.locked == 0, "locked is %u", lk.locked);
    CHECK("spin_trylock takes a free lock", spin_trylock(&lk) == 1 && lk.locked != 0, "trylock failed or the word is %u", lk.locked);
    CHECK("spin_trylock fails on a held lock without waiting", spin_trylock(&lk) == 0 && spin_trylock(&lk) == 0, "a second trylock succeeded");
    spin_unlock(&lk);
    CHECK("spin_unlock frees the lock", lk.locked == 0 && spin_trylock(&lk) == 1, "locked is %u", lk.locked);
    spin_unlock(&lk);
    spin_lock(&lk);
    CHECK("spin_lock takes a free lock at once", lk.locked != 0, "locked is %u", lk.locked);
    spin_unlock(&lk);
    CHECK("a lock can be taken and released many times", ({ int ok = 1; for (int i = 0; i < 1000; i++) { spin_lock(&lk); spin_unlock(&lk); } ok && lk.locked == 0; }), "lock word is %u", lk.locked);

    t_cli();
    uint64_t fl = spin_lock_irqsave(&lk);
    CHECK("spin_lock_irqsave holds the lock and returns the old flags", lk.locked != 0 && (fl & (1 << 9)) == 0 && !irq_enabled(), "locked %u, flags 0x%lx", lk.locked, fl);
    spin_unlock_irqrestore(&lk, fl);
    CHECK("spin_unlock_irqrestore releases and keeps interrupts off if they were off", lk.locked == 0 && !irq_enabled(), "locked %u, IF %d", lk.locked, irq_enabled());

    // Now with a real timer interrupt.
    t_timer_on();
    t_sti();
    int running = t_wait_ticks(5);
    CHECK("the timer ticks while interrupts are enabled", running, "only %lu ticks arrived", t_ticks);

    uint64_t before = t_ticks;
    uint64_t sflags = irq_save();
    t_burn(30000000);
    uint64_t during = t_ticks - before;
    irq_restore(sflags);
    CHECK("no timer tick gets through between irq_save and irq_restore", during == 0, "%lu ticks arrived while interrupts were off", during);
    CHECK("the waiting tick is delivered as soon as irq_restore enables interrupts", t_wait_ticks(before + 1), "no tick arrived after irq_restore");

    before = t_ticks;
    fl = spin_lock_irqsave(&lk);
    t_burn(30000000);
    during = t_ticks - before;
    spin_unlock_irqrestore(&lk, fl);
    CHECK("no timer tick gets through while an irqsave lock is held", during == 0 && lk.locked == 0, "%lu ticks arrived, locked %u", during, lk.locked);
    CHECK("interrupts work again after spin_unlock_irqrestore", irq_enabled() && t_wait_ticks(before + 1), "interrupts are still off or no tick came");

    t_cli();
    uint64_t pending_before = t_ticks;
    irq_wait();
    CHECK("irq_wait halts until an interrupt and returns with interrupts enabled", t_ticks > pending_before && irq_enabled(), "ticks %lu -> %lu, IF %d", pending_before, t_ticks, irq_enabled());
    t_cli();
}
