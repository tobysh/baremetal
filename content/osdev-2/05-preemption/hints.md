`irq_save`: `unsigned long f; __asm__ volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(f) : : "memory"); return f;`. `irq_restore`: `if (flags & 0x200) __asm__ volatile("sti" : : : "memory");`. Never write `sti` unconditionally.
===
`pick_next`: `int cur = current - tasks; for (int i = 1; i <= MAX_TASKS; i++) { Task *t = &tasks[(cur + i) % MAX_TASKS]; if (t->state == T_READY) return t; } return &tasks[0];`. The `<=` makes the last step land on the current slot itself.
===
In `schedule`, remember `prev = current` before changing anything, and move the old task out of `T_RUNNING` *before* calling `pick_next`: otherwise the current task is not eligible and a lone task could never be picked again.
===
In `timer_tick` the order is: count the tick, send the EOI, only then maybe `schedule()`. If your test sees `ticks` stuck at 1 or 2 after the first switch, the EOI was sent too late (or never).
===
If new tasks run once and then the machine locks up, check `task_exit`: it must mark the task `T_DEAD` *before* scheduling away, and it must not return. A trailing `for (;;) {}` keeps the compiler honest.
