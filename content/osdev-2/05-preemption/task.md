The `Task` structure, the table `tasks` (slot 0 is the boot task running `kmain`), `current`, `ticks`, `switches`, `QUANTUM`, `ctx_switch`, `sched_init`, `task_create`, `task_trampoline`, `pic_eoi()` (sends the EOI for IRQ 0) and `irq_start(hz)` are in the starter. Implement:

- `unsigned long irq_save(void)` returns the current `rflags` (`pushfq`) and disables interrupts. `void irq_restore(unsigned long flags)` enables them again only if bit 9 of `flags` is set. Pairs must nest.
- `Task *pick_next(void)` returns the first `T_READY` task found scanning from the slot after `current` and wrapping around through all `MAX_TASKS` slots (the current task's own slot is checked last). If none is ready it returns `&tasks[0]`. It changes nothing.
- `void schedule(void)` (called with interrupts off): a `T_RUNNING` current task becomes `T_READY`; picks the next task with `pick_next`; marks it `T_RUNNING` with `slice = QUANTUM`; if it is a different task, sets `current`, adds 1 to `switches` and calls `ctx_switch(&prev->sp, next->sp)`.
- `void yield(void)` is `schedule()` wrapped in `irq_save`/`irq_restore`.
- `void timer_tick(void)` is called by the interrupt dispatcher for IRQ 0 with interrupts off: adds 1 to `ticks`, sends the EOI with `pic_eoi()`, subtracts 1 from `current->slice` and calls `schedule()` when it reaches 0 or less.
- `void task_exit(void)` disables interrupts, marks the current task `T_DEAD`, calls `schedule()` and never returns.
