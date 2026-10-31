Implement `model_check` as a depth-first search over every schedule. When all threads are done and `final_ok(p, s)` is false, record `verdict = V_LOST_UPDATE`, `len` and the schedule that led there, and stop. If nothing fails, leave `verdict = V_OK` and `len = 0`.

Hidden tests replay your schedule in a separate reference interpreter and accept it only if it is legal and really ends in a failing state. They also check that correct programs (locked counters of 2 or 3 threads, a single thread, a program with no expectation) return `V_OK`, and a case where a lock is taken by only one of the two threads.

Note the sched array holds `MAX_SCHED` entries; stop descending when `depth` reaches it.
