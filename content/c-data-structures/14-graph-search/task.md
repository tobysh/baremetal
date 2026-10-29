`Edge`, `Graph`, `g_new` and `g_free` are given. Implement:

- `int g_add_edge(Graph *g, int u, int v)` adds the undirected edge `u - v`: `v` is **appended at the end** of `adj[u]` and `u` at the end of `adj[v]`. Returns `0`, or `-1` (changing nothing) if either vertex is outside `0 .. n-1` or memory runs out. Self-loops (`u == v`) add a single entry. Repeated edges are allowed and simply appear twice.
- `void bfs_dist(const Graph *g, int src, int *dist)` fills `dist[0 .. n-1]` with the number of edges on a shortest path from `src`, or `-1` for unreachable vertices. `dist[src]` is 0.
- `int dfs_order(const Graph *g, int src, int *out)` writes the vertices reachable from `src` into `out` in depth-first pre-order, visiting each vertex's neighbours in adjacency-list order, and returns how many were written. `out` has room for `n` entries.
- `int count_components(const Graph *g)` returns the number of connected components (an isolated vertex is its own component; an empty graph with `n == 0` has 0).
