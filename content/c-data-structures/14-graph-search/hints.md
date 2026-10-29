For `g_add_edge`, write a helper `append(Graph *g, int from, int to)` that walks to the end of `adj[from]` (keep a pointer to the pointer: `Edge **p = &g->adj[from]; while (*p) p = &(*p)->next;`) and stores a new `Edge` there. Call it twice (once only for a self-loop).
===
For `bfs_dist`, use an int array of size `n` as the queue (`malloc` it) with `head` and `tail` indices. Initialise all distances to -1, set `dist[src] = 0`, enqueue `src`, then process as in the lesson.
===
`dfs_order` needs a `char *seen` array (use `calloc`) and a recursive helper that appends to `out`. For `count_components`, loop `v` from 0 to `n - 1`; if `v` has not been seen, increment the counter and run a DFS from `v` that only marks `seen`. Free what you allocate.
