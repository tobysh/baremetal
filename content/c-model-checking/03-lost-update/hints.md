Keep the current path in `r->sched[depth]` as you recurse. When you find a failure, `r->len = depth` and return 1 up the stack so every caller stops.
===
Recursive helper `static int dfs(const program *p, const state *s, int depth, result *r)`; at the top handle the `all_done` case.
===
For each enabled thread: `state next = *s; step(p, &next, t); r->sched[depth] = t; if (dfs(p, &next, depth + 1, r)) return 1;`
