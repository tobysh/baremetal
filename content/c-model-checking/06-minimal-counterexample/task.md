Rewrite `model_check` so it returns the minimal counterexample: fewest steps, then lexicographically first, using the precedence above. `V_OK` has `len = 0`.

Tests compare your verdict and exact schedule with an independent exhaustive reference on: racy counters (2 and 3 threads, 1 and 2 increments each), unlocked against locked, deadlocks (2 locks, a 3-cycle, a leaked lock), overdraw races, a transient violation, and a pair where **a deadlock and a lost update are both possible** so only the shortest is correct. Correct programs must give `V_OK`.

The tests cannot grade whether you can explain the schedule; read the one you get for the racy counter and check it against the lesson.
