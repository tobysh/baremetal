#include <stdio.h>
#include <stdlib.h>

typedef struct Edge {
    int to;
    struct Edge *next;
} Edge;

typedef struct {
    int n;
    Edge **adj;
} Graph;

Graph *g_new(int n) {
    Graph *g = malloc(sizeof *g);
    if (g == NULL) return NULL;
    g->n = n;
    g->adj = calloc((size_t)(n > 0 ? n : 1), sizeof *g->adj);
    if (g->adj == NULL) {
        free(g);
        return NULL;
    }
    return g;
}

void g_free(Graph *g) {
    if (g == NULL) return;
    for (int v = 0; v < g->n; v++) {
        Edge *e = g->adj[v];
        while (e) {
            Edge *next = e->next;
            free(e);
            e = next;
        }
    }
    free(g->adj);
    free(g);
}

static Edge **tail_link(Graph *g, int from) {
    Edge **p = &g->adj[from];
    while (*p) p = &(*p)->next;
    return p;
}

int g_add_edge(Graph *g, int u, int v) {
    if (u < 0 || u >= g->n || v < 0 || v >= g->n) return -1;
    Edge *a = malloc(sizeof *a);
    if (a == NULL) return -1;
    Edge *b = NULL;
    if (u != v) {
        b = malloc(sizeof *b);
        if (b == NULL) {
            free(a);
            return -1;
        }
    }
    a->to = v;
    a->next = NULL;
    *tail_link(g, u) = a;
    if (b) {
        b->to = u;
        b->next = NULL;
        *tail_link(g, v) = b;
    }
    return 0;
}

void bfs_dist(const Graph *g, int src, int *dist) {
    for (int v = 0; v < g->n; v++) dist[v] = -1;
    if (src < 0 || src >= g->n) return;
    int *queue = malloc((size_t)g->n * sizeof *queue);
    if (queue == NULL) return;
    int head = 0, tail = 0;
    dist[src] = 0;
    queue[tail++] = src;
    while (head < tail) {
        int u = queue[head++];
        for (Edge *e = g->adj[u]; e; e = e->next) {
            if (dist[e->to] == -1) {
                dist[e->to] = dist[u] + 1;
                queue[tail++] = e->to;
            }
        }
    }
    free(queue);
}

static void dfs(const Graph *g, int u, char *seen, int *out, int *count) {
    seen[u] = 1;
    if (out) out[(*count)++] = u;
    for (Edge *e = g->adj[u]; e; e = e->next)
        if (!seen[e->to]) dfs(g, e->to, seen, out, count);
}

int dfs_order(const Graph *g, int src, int *out) {
    if (src < 0 || src >= g->n) return 0;
    char *seen = calloc((size_t)g->n, 1);
    if (seen == NULL) return 0;
    int count = 0;
    dfs(g, src, seen, out, &count);
    free(seen);
    return count;
}

int count_components(const Graph *g) {
    if (g->n <= 0) return 0;
    char *seen = calloc((size_t)g->n, 1);
    if (seen == NULL) return 0;
    int comps = 0, dummy = 0;
    for (int v = 0; v < g->n; v++) {
        if (!seen[v]) {
            comps++;
            dfs(g, v, seen, NULL, &dummy);
        }
    }
    free(seen);
    return comps;
}

int main(void) {
    Graph *g = g_new(5);
    g_add_edge(g, 0, 1);
    g_add_edge(g, 1, 2);
    g_add_edge(g, 3, 4);
    printf("components: %d (expected 2)\n", count_components(g));
    g_free(g);
    return 0;
}
