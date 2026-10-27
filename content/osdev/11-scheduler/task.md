`Task`, `TaskState` and `MAX_TASKS` (8) are in the starter. Implement:

- `void sched_init(Task *idle)` resets the scheduler: empty table, clock at 0, and `idle` becomes the current task in state `TASK_RUNNING`. The idle task is never stored in the table.
- `int sched_add(Task *t)` sets `t->state = TASK_READY` and stores `t` in the lowest-numbered slot that is empty or holds a `TASK_DEAD` task. Returns 0, or -1 if every slot holds a live task.
- `Task *sched_current(void)` returns the current task.
- `Task *sched_pick_next(void)` chooses the next task and returns it. If the current task is `TASK_RUNNING` it becomes `TASK_READY`; any other state is left alone. Then, starting at the slot after the current task's slot (at slot 0 if the current task is idle), and wrapping around once through all slots including the current task's own, it takes the first `TASK_READY` task. If there is none, it takes the idle task. The chosen task becomes `TASK_RUNNING` and current.
- `void sched_sleep(uint64_t ticks)` puts the current task in `TASK_SLEEPING` with `wake_tick = now + ticks`. Does nothing if the current task is idle.
- `void sched_tick(void)` adds 1 to the clock, then makes every `TASK_SLEEPING` task with `wake_tick <= now` `TASK_READY`.
- `uint64_t sched_now(void)` returns the clock.
- `void sched_block(void)` puts the current task in `TASK_BLOCKED` (not idle). `void sched_unblock(Task *t)` makes `t` `TASK_READY`, but only if it is `TASK_BLOCKED`.
- `void sched_exit(void)` puts the current task in `TASK_DEAD` (not idle).
