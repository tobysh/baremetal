Keep `static Task *slots[MAX_TASKS]; static Task *current; static Task *idle_task; static int current_slot; static uint64_t now;`. `current_slot` is -1 when the current task is idle.
===
Setting `current_slot` is the easy part to forget: set it in `sched_pick_next` whenever you choose a task (the slot index, or -1 for idle).
===
The search loop: `for (int i = 1; i <= MAX_TASKS; i++) { int s = (start + i) % MAX_TASKS; ... }` where `start` is `current_slot` or `-1`. With `start = -1` the first slot checked is `(−1 + 1) % 8 = 0`. The loop covers every slot once, finishing on the current task's own slot.
===
Check for an empty slot (`slots[s] == NULL`) before looking at the state. A dead task stays in its slot until `sched_add` reuses it, but it is never READY, so the search skips it.
===
Tick: `now++; for each slot with a task: if (t->state == TASK_SLEEPING && t->wake_tick <= now) t->state = TASK_READY;`
