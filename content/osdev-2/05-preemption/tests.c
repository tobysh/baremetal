static int if_set(void) { unsigned long f; __asm__ volatile("pushfq\n\tpopq %0" : "=r"(f)); return (f & 0x200) != 0; }
static uint64_t spin_wait_ticks(uint64_t target) {
    for (unsigned long spins = 0; ticks < target && spins < 40000000UL; spins++) __asm__ volatile("pause");
    return ticks;
}

static char trace[64];
static int trace_len;
static volatile int if_in_task;
static void tr_a(void) { for (int i = 0; i < 3; i++) { trace[trace_len++] = 'A'; if_in_task = if_set(); yield(); } }
static void tr_b(void) { for (int i = 0; i < 3; i++) { trace[trace_len++] = 'B'; yield(); } }
static void tr_c(void) { for (int i = 0; i < 3; i++) { trace[trace_len++] = 'C'; yield(); } }
static void nop_task(void) { for (;;) yield(); }

static volatile uint64_t count[3];
static volatile int stop;
static void spin0(void) { while (!stop) count[0]++; }
static void spin1(void) { while (!stop) count[1]++; }
static void spin2(void) { while (!stop) count[2]++; }

void kmain(void) {
    bm_outb(0x21, 0xFF); bm_outb(0xA1, 0xFF);          // no IRQs yet
    __asm__ volatile("cli");

    unsigned long f = irq_save();
    CHECK("irq_save returns rflags without IF when interrupts were already off", (f & 0x200) == 0 && !if_set(), "flags=0x%lx", f);
    irq_restore(f);
    CHECK("irq_restore does not turn interrupts on if they were off", !if_set(), "interrupts were enabled by irq_restore");
    __asm__ volatile("sti");
    f = irq_save();
    CHECK("irq_save returns the old flags (IF set) and disables interrupts", (f & 0x200) != 0 && !if_set(), "flags=0x%lx IF now %d", f, if_set());
    irq_restore(f);
    CHECK("irq_restore turns interrupts back on", if_set(), "interrupts stayed off");
    unsigned long f1 = irq_save(), f2 = irq_save();
    irq_restore(f2);
    int mid = if_set();
    irq_restore(f1);
    CHECK("nested save/restore pairs: the inner restore keeps interrupts off, the outer turns them on", !mid && if_set(), "inner restore left IF=%d, outer left IF=%d", mid, if_set());
    __asm__ volatile("cli");

    sched_init();
    Task *t1 = task_create(nop_task), *t2 = task_create(nop_task), *t3 = task_create(nop_task);
    CHECK("task_create fills slots 1, 2, 3", t1 == &tasks[1] && t2 == &tasks[2] && t3 == &tasks[3], "tasks at %ld %ld %ld", (long)(t1 - tasks), (long)(t2 - tasks), (long)(t3 - tasks));
    CHECK("pick_next from the boot task chooses slot 1", pick_next() == t1, "chose slot %ld", (long)(pick_next() - tasks));
    CHECK("pick_next changes nothing", tasks[0].state == T_RUNNING && t1->state == T_READY, "states changed");
    current = t1; t1->state = T_RUNNING;
    CHECK("from slot 1 it chooses slot 2", pick_next() == t2, "chose slot %ld", (long)(pick_next() - tasks));
    t2->state = T_BLOCKED;
    CHECK("it skips blocked tasks", pick_next() == t3, "chose slot %ld", (long)(pick_next() - tasks));
    current = t3; t3->state = T_RUNNING; tasks[0].state = T_READY;
    CHECK("it wraps around to the boot task in slot 0", pick_next() == &tasks[0], "chose slot %ld", (long)(pick_next() - tasks));
    t1->state = T_DEAD; tasks[0].state = T_BLOCKED; t3->state = T_RUNNING;
    CHECK("with nothing else ready, a running current task is picked again? no: slot 0 is the fallback", pick_next() == &tasks[0], "chose slot %ld", (long)(pick_next() - tasks));

    // Cooperative round robin through yield(), still without a timer.
    sched_init();
    trace_len = 0; memset(trace, 0, sizeof trace);
    task_create(tr_a); task_create(tr_b); task_create(tr_c);
    for (int i = 0; i < 30 && !(tasks[1].state == T_DEAD && tasks[2].state == T_DEAD && tasks[3].state == T_DEAD); i++) yield();
    CHECK("three tasks that yield alternate in creation order", trace_len == 9 && memcmp(trace, "ABCABCABC", 9) == 0, "trace was \"%s\"", trace);
    CHECK("a task that returns from its entry function ends up dead", tasks[1].state == T_DEAD && tasks[2].state == T_DEAD && tasks[3].state == T_DEAD, "states %d %d %d", tasks[1].state, tasks[2].state, tasks[3].state);
    CHECK("new tasks start with interrupts enabled (the trampoline ran sti)", if_in_task == 1, "IF was %d inside the task", if_in_task);
    CHECK("the boot task got its own interrupt flag back after each yield", !if_set(), "yield left interrupts enabled in the boot task");
    CHECK("switches counts every change of task", switches >= 9, "switches=%lu", (unsigned long)switches);
    Task *again = task_create(nop_task);
    CHECK("a dead task's slot is reused", again == &tasks[1], "got slot %ld", (long)(again - tasks));

    // Preemption: three tasks that never yield.
    sched_init();
    ticks = 0; stop = 0;
    for (int i = 0; i < 3; i++) count[i] = 0;
    task_create(spin0); task_create(spin1); task_create(spin2);
    irq_start(1000);
    uint64_t got = spin_wait_ticks(60);
    CHECK("timer ticks keep arriving while tasks switch (EOI sent before switching)", got >= 60, "only %lu ticks arrived", (unsigned long)got);
    uint64_t c0 = count[0], c1 = count[1], c2 = count[2];
    CHECK("all three spinning tasks have run, although none of them yields", c0 > 0 && c1 > 0 && c2 > 0, "counters %lu %lu %lu", (unsigned long)c0, (unsigned long)c1, (unsigned long)c2);
    uint64_t mx = c0 > c1 ? c0 : c1; if (c2 > mx) mx = c2;
    uint64_t mn = c0 < c1 ? c0 : c1; if (c2 < mn) mn = c2;
    CHECK("the CPU is shared roughly fairly", mn * 10 >= mx, "counters %lu %lu %lu", (unsigned long)c0, (unsigned long)c1, (unsigned long)c2);
    CHECK("the scheduler switched several times", switches >= 6, "switches=%lu", (unsigned long)switches);
    CHECK("the boot task was preempted too and got the CPU back", ticks >= 60, "the boot task never ran again");

    stop = 1;
    for (unsigned long spins = 0; spins < 40000000UL && !(tasks[1].state == T_DEAD && tasks[2].state == T_DEAD && tasks[3].state == T_DEAD); spins++) __asm__ volatile("pause");
    CHECK("tasks that return from a preempted run exit cleanly", tasks[1].state == T_DEAD && tasks[2].state == T_DEAD && tasks[3].state == T_DEAD, "states %d %d %d", tasks[1].state, tasks[2].state, tasks[3].state);
    __asm__ volatile("cli");
}
