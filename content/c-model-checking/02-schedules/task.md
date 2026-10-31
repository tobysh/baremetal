Implement in the starter:

- `run_schedule(p, sched, n, out)`: initialise `*out`, apply the first `n` steps of `sched`. Return `n` if all were legal, otherwise `-(i+1)` for the first illegal step `i` (thread id out of range, finished, or blocked). `out` holds the state reached so far.
- `count_schedules(p)`: the number of different thread-id sequences that run every thread to completion. Dead ends do not count.

Tests use thread lengths 3/3, 3/3/3 and 3/6 (known counts), locked programs and a deadlocking program (compared to an independent reference count) and 300 random schedules, legal and not, compared to a reference replay.
