Replace the iterative deepening with a breadth-first search over **distinct** states. Set `r->states` to the number of distinct states discovered. Verdicts and minimal counterexamples must stay exactly as in lesson 6.

Tests check `states` against an independent reachable-state count for locked counters, locked withdrawals and CAS retry-loop counters (including three threads with two increments each), check verdicts and schedules against a reference for failing programs, and require retry loops to terminate with `V_OK`. Your lesson 6 search gives up on the retry loops, so the starter fails.
