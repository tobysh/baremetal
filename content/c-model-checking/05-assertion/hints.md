Test the invariant at the top of the recursion, before anything else, so the initial state and every later state are covered.
===
Put it before the `all_done` check. A state can be both finished and in violation, and the violation is the more specific report.
===
Do not wait for the end of the schedule: the transient program is only caught if you look at the state right after the write.
