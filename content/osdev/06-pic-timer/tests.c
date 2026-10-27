static uint64_t rdtsc(void) { uint32_t lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }

// Waits (with interrupts on, bounded so a missing tick cannot hang the test) until `target` ticks have happened.
static int wait_for(uint64_t target) {
    for (unsigned long spins = 0; timer_ticks() < target; spins++) {
        if (spins > 40000000UL) return 0;
        __asm__ volatile("pause");
    }
    return 1;
}

void kmain(void) {
    bm_outb(0x21, 0xA5);
    bm_outb(0xA1, 0x5A);
    pic_remap(32, 40);
    uint8_t m1 = bm_inb(0x21), m2 = bm_inb(0xA1);
    CHECK("pic_remap keeps the master's interrupt mask", m1 == 0xA5, "master mask is 0x%02x, was 0xA5", m1);
    CHECK("pic_remap keeps the slave's interrupt mask", m2 == 0x5A, "slave mask is 0x%02x, was 0x5A", m2);

    bm_outb(0x21, 0xFF); bm_outb(0xA1, 0xFF);
    pic_set_mask(0, 0);
    CHECK("pic_set_mask(0, 0) unmasks IRQ 0 on the master only", bm_inb(0x21) == 0xFE && bm_inb(0xA1) == 0xFF, "masks are 0x%02x 0x%02x", bm_inb(0x21), bm_inb(0xA1));
    pic_set_mask(11, 0);
    CHECK("pic_set_mask(11, 0) unmasks IRQ 11 on the slave (bit 3)", bm_inb(0xA1) == 0xF7 && bm_inb(0x21) == 0xFE, "masks are 0x%02x 0x%02x", bm_inb(0x21), bm_inb(0xA1));
    pic_set_mask(11, 1);
    pic_set_mask(0, 1);
    CHECK("pic_set_mask(..., 1) masks again", bm_inb(0x21) == 0xFF && bm_inb(0xA1) == 0xFF, "masks are 0x%02x 0x%02x", bm_inb(0x21), bm_inb(0xA1));

    timer_init(100);
    int ok = wait_for(10);
    CHECK("timer interrupts arrive: ten ticks counted", ok,
          "only %lu ticks arrived. IRQ 0 must be remapped to vector 32, unmasked, and acknowledged with an EOI", (unsigned long)timer_ticks());
    if (!ok) return;

    uint64_t isr;
    bm_outb(PIC1_CMD, 0x0B);
    isr = bm_inb(PIC1_CMD);
    CHECK("the master has nothing left in service between ticks", (isr & 0x01) == 0, "ISR is 0x%02x", (unsigned)isr);
    CHECK("only IRQ 0 is unmasked", bm_inb(0x21) == 0xFE && bm_inb(0xA1) == 0xFF, "masks are 0x%02x 0x%02x", bm_inb(0x21), bm_inb(0xA1));

    // Frequency: the same number of ticks at 100 Hz should take about ten times as long as at 1000 Hz.
    uint64_t t0 = timer_ticks(), c0 = rdtsc();
    wait_for(t0 + 20);
    uint64_t slow = rdtsc() - c0;
    pit_init(1000);
    t0 = timer_ticks(); c0 = rdtsc();
    ok = wait_for(t0 + 20);
    uint64_t fast = rdtsc() - c0;
    unsigned ratio = fast ? (unsigned)(slow / fast) : 0;
    CHECK("pit_init(hz) sets the tick rate: 1000 Hz is about 10 times faster than 100 Hz", ok && ratio >= 4 && ratio <= 25, "ratio was %u (expected about 10)", ratio);

    t0 = timer_ticks();
    sleep_ticks(0);
    CHECK("sleep_ticks(0) returns immediately", timer_ticks() - t0 <= 1, "waited %lu ticks", (unsigned long)(timer_ticks() - t0));
    t0 = timer_ticks();
    sleep_ticks(20);
    uint64_t waited = timer_ticks() - t0;
    CHECK("sleep_ticks(20) waits at least 20 ticks", waited >= 20, "waited %lu ticks", (unsigned long)waited);
    CHECK("sleep_ticks(20) does not wait much longer", waited <= 40, "waited %lu ticks", (unsigned long)waited);
}
