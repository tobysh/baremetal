Extend `model_check` so that a state with unfinished threads and no enabled thread gives `verdict = V_DEADLOCK`, with `len` and `sched` the steps that reached it. Lost updates must still be reported as `V_LOST_UPDATE`.

Tests: opposite order (2 locks), a three-thread cycle, the same cycle broken by lock order, a lock leaked by a finished thread, and the earlier lost-update and locked-counter programs. Every schedule you return is replayed in a reference interpreter and must end in a state that is stuck.
