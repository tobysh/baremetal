static void t_inject(uint8_t sc) {
    bm_outb(0x64, 0xD2);
    bm_outb(0x60, sc);
}

static int t_wait_count(unsigned n) {
    for (unsigned long spins = 0; ring_count(&kbd_ring) < n; spins++) {
        if (spins > 3000000UL) return 0;
        __asm__ volatile("pause");
    }
    return 1;
}

static int t_wait_ticks(uint64_t target) {
    for (unsigned long spins = 0; ticks < target; spins++) {
        if (spins > 60000000UL) return 0;
        __asm__ volatile("pause");
    }
    return 1;
}

static void (*t_real)(uint64_t, uint64_t, uint64_t);
static volatile int t_timer_hits;
static void t_wrap(uint64_t vec, uint64_t err, uint64_t rip) {
    t_real(vec, err, rip);
    if (vec == 32 && ++t_timer_hits == 4) t_inject(0x2E);
}

void kmain(void) {
    Ring r;
    uint8_t b = 0x77;
    memset(&r, 0xEE, sizeof r);
    ring_init(&r);
    CHECK("ring_init empties the ring", ring_count(&r) == 0 && r.head == r.tail, "count is %u", ring_count(&r));
    CHECK("ring_pop on an empty ring returns 0 and leaves the output alone", ring_pop(&r, &b) == 0 && b == 0x77, "returned a byte or changed it (0x%02x)", b);

    CHECK("ring_push stores a byte", ring_push(&r, 10) == 1 && ring_count(&r) == 1, "count is %u", ring_count(&r));
    CHECK("ring_pop returns it", ring_pop(&r, &b) == 1 && b == 10 && ring_count(&r) == 0, "got %u, count %u", b, ring_count(&r));

    int ok = 1;
    for (int i = 0; i < RING_SIZE; i++) if (!ring_push(&r, (uint8_t)(100 + i))) ok = 0;
    CHECK("a ring accepts exactly RING_SIZE bytes", ok && ring_count(&r) == RING_SIZE, "count is %u", ring_count(&r));
    CHECK("pushing into a full ring returns 0 and keeps the old data", ring_push(&r, 0xFF) == 0 && ring_count(&r) == RING_SIZE, "count is %u", ring_count(&r));
    ok = 1;
    for (int i = 0; i < RING_SIZE; i++) if (!ring_pop(&r, &b) || b != 100 + i) ok = 0;
    CHECK("bytes come out in the order they went in (FIFO)", ok && ring_count(&r) == 0, "order or count wrong, last byte %u", b);

    ok = 1;
    for (int round = 0; round < 50; round++) {
        for (int i = 0; i < 5; i++) if (!ring_push(&r, (uint8_t)(round * 7 + i))) ok = 0;
        for (int i = 0; i < 5; i++) if (!ring_pop(&r, &b) || b != (uint8_t)(round * 7 + i)) ok = 0;
    }
    CHECK("the ring wraps around the array many times", ok && ring_count(&r) == 0, "a round trip lost or reordered data");

    ring_init(&r);
    r.head = r.tail = 0xFFFFFFF9u;
    ok = 1;
    for (int i = 0; i < RING_SIZE; i++) if (!ring_push(&r, (uint8_t)(i * 3))) ok = 0;
    CHECK("the counters may wrap past 2^32 while the ring is full", ok && ring_count(&r) == RING_SIZE && ring_push(&r, 1) == 0, "count is %u", ring_count(&r));
    ok = 1;
    for (int i = 0; i < RING_SIZE; i++) if (!ring_pop(&r, &b) || b != (uint8_t)(i * 3)) ok = 0;
    CHECK("pops are correct across the 2^32 wrap", ok && ring_count(&r) == 0, "data differs after the wrap");

    CHECK("kbd_read before keyboard_init has nothing", ({ kbd_ring.head = kbd_ring.tail = 0; kbd_read() == -1; }), "returned a byte");

    keyboard_init();
    CHECK("keyboard_init enables interrupts", ({ uint64_t f; __asm__ volatile("pushfq; popq %0" : "=r"(f)); (f & (1 << 9)) != 0; }), "IF is clear");
    CHECK("keyboard_init masks everything except IRQ 1", bm_inb(0x21) == 0xFD && bm_inb(0xA1) == 0xFF, "masks are 0x%02x 0x%02x", bm_inb(0x21), bm_inb(0xA1));
    CHECK("the ring starts empty", ring_count(&kbd_ring) == 0, "count is %u", ring_count(&kbd_ring));

    t_inject(0x1E);
    int got = t_wait_count(1);
    CHECK("a key press travels through the IRQ into the ring", got, "no byte arrived (IRQ 1 must be remapped to vector 33, unmasked and handled)");
    CHECK("the byte in the ring is the scancode that was sent", kbd_read() == 0x1E, "kbd_read did not return 0x1E");
    CHECK("kbd_read returns -1 when the ring is empty", kbd_read() == -1, "returned a byte");

    // A second and third key only arrive if the handler sent the EOI.
    t_inject(0x9E);
    got = t_wait_count(1);
    t_inject(0xB0);
    got = got && t_wait_count(2);
    CHECK("more keys keep arriving, so the handler acknowledges the IRQ", got, "only %u bytes arrived; missing EOI?", ring_count(&kbd_ring));
    int first = kbd_read(), second = kbd_read();
    CHECK("kbd_read returns bytes above 127 as positive numbers, in order", first == 0x9E && second == 0xB0, "got %d and %d", first, second);

    // Fill the ring from the interrupt handler: the 17th and later bytes are dropped.
    int sent = 0;
    for (; sent < 20; sent++) {
        t_inject((uint8_t)(0x10 + sent));
        if (!t_wait_count(sent + 1 > RING_SIZE ? RING_SIZE : (unsigned)(sent + 1))) break;
        if (sent >= RING_SIZE) for (volatile int d = 0; d < 200000; d++) { }
    }
    ok = ring_count(&kbd_ring) == RING_SIZE;
    for (int i = 0; i < RING_SIZE; i++) if (kbd_read() != 0x10 + i) ok = 0;
    CHECK("when the ring is full later keys are dropped, the oldest 16 survive", ok && sent == 20, "sent %d, ring did not keep the first %d keys in order", sent, RING_SIZE);
    CHECK("the ring is empty again after draining it", kbd_read() == -1, "extra byte");

    // kbd_read keeps interrupts as they were.
    __asm__ volatile("cli");
    kbd_read();
    uint64_t fl;
    __asm__ volatile("pushfq; popq %0" : "=r"(fl));
    CHECK("kbd_read does not turn interrupts on if they were off", (fl & (1 << 9)) == 0, "IF is set");
    __asm__ volatile("sti");
    kbd_read();
    __asm__ volatile("pushfq; popq %0" : "=r"(fl));
    CHECK("kbd_read leaves interrupts on if they were on", (fl & (1 << 9)) != 0, "IF is clear");

    // kbd_wait: a key already waiting.
    t_inject(0x2C);
    t_wait_count(1);
    CHECK("kbd_wait returns a byte that is already there", kbd_wait() == 0x2C, "wrong byte");

    // kbd_wait must really sleep: the key arrives from inside the fourth timer tick.
    t_real = interrupt_handler;
    bm_exception_hook = t_wrap;
    pic_set_mask(0, 0);
    pit_init(500);
    t_timer_hits = 0;
    uint8_t w = kbd_wait();
    pic_set_mask(0, 1);
    CHECK("kbd_wait sleeps until a key arrives and then returns it", w == 0x2E && t_timer_hits >= 4, "returned 0x%02x after %d ticks", w, t_timer_hits);
    CHECK("kbd_wait returns with interrupts enabled", ({ __asm__ volatile("pushfq; popq %0" : "=r"(fl)); (fl & (1 << 9)) != 0; }), "IF is clear");
    CHECK("the timer handler counted ticks while kbd_wait slept", ticks >= 4, "ticks is %lu", (unsigned long)ticks);
    (void)t_wait_ticks;
    __asm__ volatile("cli");
}
