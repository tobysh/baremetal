`state_init(p, out)` first. Then a loop over the schedule that checks `thread_enabled` before each `step`.
===
For counting, write a recursive helper `long count_from(const program *p, const state *s)`.
===
If `all_done(p, s)` the answer is 1. Otherwise add up `count_from` over every enabled thread, each on a copy of the state after one step. Nothing enabled and not done gives 0 automatically.
