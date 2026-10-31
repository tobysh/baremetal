Write `ks_check` one invariant at a time, each as its own loop, and test it with the starter's `main` by corrupting one field.
===
For `V_REFCOUNT`, first fill a `count[NOFD]` array by scanning every slot of every pid, then compare it with each description's `refs`. Only descriptions with `refs > 0` are compared to the count, and `refs < 0` is always a violation.
===
For `V_CYCLE`, follow `ppid` at most `NPROC` times. Stop quietly when you reach pid 1, a free slot or an out-of-range pid. Only running out of steps is a cycle.
===
In `ks_exit`, ask `pt_exit` first: it already rejects pid 1, free and zombie processes. Read the parent's pid from the table after the call; the exiting process keeps its own `ppid`.
