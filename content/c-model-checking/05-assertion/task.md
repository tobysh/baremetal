Extend `model_check` so a state that fails `invariant_ok` gives `verdict = V_ASSERT`, with the schedule that reached it (for the initial state, `len = 0`). Keep reporting lost updates and deadlocks.

Tests: the overdraw race with 2 and 3 withdrawing threads, the locked version with 2 and 3 threads (clean), a single thread with a transient violation, a violated initial state, a program with no invariant, and the earlier programs. Each reported schedule is replayed in a reference interpreter.
