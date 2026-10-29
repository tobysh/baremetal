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

int g_add_edge(Graph *g, int u, int v) {
    return -1;
}

void bfs_dist(const Graph *g, int src, int *dist) {
}

int dfs_order(const Graph *g, int src, int *out) {
    return 0;
}

int count_components(const Graph *g) {
    return 0;
}

// Optional: try it with Run.
int main(void) {
    Graph *g = g_new(5);
    g_add_edge(g, 0, 1);
    g_add_edge(g, 1, 2);
    g_add_edge(g, 3, 4);
    printf("components: %d (expected 2)\n", count_components(g));
    g_free(g);
    return 0;
}
