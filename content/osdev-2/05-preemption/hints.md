Preemption means the timer interrupt can switch tasks at any instruction, so shared scheduler state must only be touched with interrupts off, and the code that disables them must put back the state it found, not always enable. Round-robin means picking the next ready task after the current one, wrapping around.
===
Look at each function's contract in the task: `irq_save` and `irq_restore` use the saved rflags, `pick_next` only reads, `schedule` does the state changes and calls `ctx_switch`, and `timer_tick` and `task_exit` are thin callers of `schedule`. The `ctx_switch` signature in the starter shows what it expects to be saved and loaded.
===
Typical mistakes: unconditional `sti` in `irq_restore` (nested sections re-enable too early), forgetting to mark the old task ready before picking (a lone task is never chosen again), losing the previous task pointer after updating `current`, sending the EOI after `schedule()` (the PIC stays blocked while another task runs), and a `task_exit` that returns or is not marked dead before switching away.
===
Wrap-around scan on a different table: `for (int i = 1; i <= N; i++) if (ok(&t[(start + i) % N])) return i;` visits every slot once, starting after `start`.
