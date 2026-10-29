static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    Graph *g = g_new(8);
    CHECK("g_add_edge rejects bad vertices", g_add_edge(g, -1, 2) == -1 && g_add_edge(g, 0, 8) == -1 && g_add_edge(g, 9, 9) == -1, "expected -1");
    CHECK("a rejected edge changes nothing", g->adj[0] == NULL && g->adj[2] == NULL, "an edge was added");
    int pairs[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}, {5, 6}};
    int ok = 1;
    for (int i = 0; i < 6; i++) if (g_add_edge(g, pairs[i][0], pairs[i][1]) != 0) ok = 0;
    CHECK("add six edges", ok, "an add failed");
    CHECK("edges go both ways", g->adj[0] && g->adj[0]->to == 1 && g->adj[1] && g->adj[1]->to == 0 && g->adj[4] && g->adj[4]->to == 3, "reverse entry missing");
    CHECK("edges are appended in order: vertex 0 lists 1 then 2", g->adj[0] && g->adj[0]->next && g->adj[0]->to == 1 && g->adj[0]->next->to == 2 && g->adj[0]->next->next == NULL, "wrong order for vertex 0");
    CHECK("vertex 3 lists 1, 2, 4 in order", g->adj[3] && g->adj[3]->to == 1 && g->adj[3]->next && g->adj[3]->next->to == 2 && g->adj[3]->next->next && g->adj[3]->next->next->to == 4, "wrong order for vertex 3");

    int dist[8];
    bfs_dist(g, 0, dist);
    int want[] = {0, 1, 1, 2, 3, -1, -1, -1};
    ok = 1;
    for (int i = 0; i < 8; i++) if (dist[i] != want[i]) ok = 0;
    CHECK("BFS distances from 0", ok, "got %d %d %d %d %d %d %d %d", dist[0], dist[1], dist[2], dist[3], dist[4], dist[5], dist[6], dist[7]);
    bfs_dist(g, 4, dist);
    CHECK("BFS distances from 4", dist[4] == 0 && dist[3] == 1 && dist[1] == 2 && dist[2] == 2 && dist[0] == 3 && dist[7] == -1, "got %d %d %d %d %d", dist[0], dist[1], dist[2], dist[3], dist[4]);
    bfs_dist(g, 7, dist);
    CHECK("BFS from an isolated vertex", dist[7] == 0 && dist[0] == -1 && dist[5] == -1, "got %d %d", dist[7], dist[0]);

    int order[8];
    int n = dfs_order(g, 0, order);
    int dw[] = {0, 1, 3, 2, 4};
    ok = n == 5;
    for (int i = 0; ok && i < 5; i++) if (order[i] != dw[i]) ok = 0;
    CHECK("DFS pre-order from 0 is 0 1 3 2 4", ok, "returned %d: %d %d %d %d %d", n, order[0], order[1], order[2], order[3], order[4]);
    n = dfs_order(g, 4, order);
    int dw2[] = {4, 3, 1, 0, 2};
    ok = n == 5;
    for (int i = 0; ok && i < 5; i++) if (order[i] != dw2[i]) ok = 0;
    CHECK("DFS pre-order from 4 is 4 3 1 0 2", ok, "returned %d: %d %d %d %d %d", n, order[0], order[1], order[2], order[3], order[4]);
    CHECK("DFS from an isolated vertex returns only it", dfs_order(g, 7, order) == 1 && order[0] == 7, "wrong");
    CHECK("DFS stays inside its component", dfs_order(g, 5, order) == 2 && order[0] == 5 && order[1] == 6, "wrong");

    CHECK("count_components: {0-4}, {5,6}, {7} is 3", count_components(g) == 3, "got %d", count_components(g));
    g_add_edge(g, 4, 5);
    CHECK("joining two components leaves 2", count_components(g) == 2, "got %d", count_components(g));
    g_add_edge(g, 7, 7);
    CHECK("a self-loop does not merge anything", count_components(g) == 2 && g->adj[7] && g->adj[7]->next == NULL, "got %d", count_components(g));
    g_free(g);

    Graph *cyc = g_new(4);
    g_add_edge(cyc, 0, 1);
    g_add_edge(cyc, 1, 2);
    g_add_edge(cyc, 2, 3);
    g_add_edge(cyc, 3, 0);
    g_add_edge(cyc, 0, 1);
    int co[4];
    int cn = dfs_order(cyc, 0, co);
    CHECK("DFS visits each vertex once on a cycle with a repeated edge", cn == 4 && co[0] == 0 && co[1] == 1 && co[2] == 2 && co[3] == 3, "returned %d", cn);
    bfs_dist(cyc, 0, dist);
    CHECK("BFS on a 4-cycle: 0 1 2 1", dist[0] == 0 && dist[1] == 1 && dist[2] == 2 && dist[3] == 1, "got %d %d %d %d", dist[0], dist[1], dist[2], dist[3]);
    g_free(cyc);

    Graph *empty = g_new(0);
    CHECK("a graph with no vertices has 0 components", count_components(empty) == 0, "got %d", count_components(empty));
    g_free(empty);

    unsigned seed = 271828u;
    ok = 1;
    int trial = 0;
    for (; trial < 40 && ok; trial++) {
        int V = 1 + (int)(rng(&seed) % 20u);
        int E = (int)(rng(&seed) % 30u);
        Graph *r = g_new(V);
        int d[20][20];
        for (int i = 0; i < V; i++) for (int j = 0; j < V; j++) d[i][j] = (i == j) ? 0 : 1000000;
        for (int k = 0; k < E; k++) {
            int u = (int)(rng(&seed) % (unsigned)V), v = (int)(rng(&seed) % (unsigned)V);
            if (g_add_edge(r, u, v) != 0) ok = 0;
            if (u != v) { d[u][v] = 1; d[v][u] = 1; }
        }
        for (int k = 0; k < V; k++) for (int i = 0; i < V; i++) for (int j = 0; j < V; j++)
            if (d[i][k] + d[k][j] < d[i][j]) d[i][j] = d[i][k] + d[k][j];
        int comps = 0;
        int rep[20];
        for (int i = 0; i < V; i++) { rep[i] = 1; for (int j = 0; j < i; j++) if (d[i][j] < 1000000) rep[i] = 0; comps += rep[i]; }
        if (count_components(r) != comps) ok = 0;
        for (int s = 0; s < V; s++) {
            int got[20], ord[20];
            bfs_dist(r, s, got);
            int reach = 0;
            for (int j = 0; j < V; j++) {
                int w = d[s][j] >= 1000000 ? -1 : d[s][j];
                if (got[j] != w) ok = 0;
                if (w >= 0) reach++;
            }
            int cnt = dfs_order(r, s, ord);
            if (cnt != reach || ord[0] != s) ok = 0;
            char dup[20] = {0};
            for (int i = 0; i < cnt && ok; i++) {
                if (ord[i] < 0 || ord[i] >= V || dup[ord[i]] || d[s][ord[i]] >= 1000000) ok = 0;
                else dup[ord[i]] = 1;
            }
        }
        g_free(r);
    }
    CHECK("40 random graphs agree with Floyd-Warshall (distances, reachability, components)", ok, "failed on trial %d", trial);
    return 0;
}
