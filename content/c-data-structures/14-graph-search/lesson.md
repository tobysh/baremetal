---
title: Graphs, BFS and DFS
summary: Model connections with an adjacency list. Write breadth-first search for shortest paths, depth-first search for visit order, and count connected components.
minutes: 50
difficulty: hard
tags: graphs, queues, recursion, data-structures
id: c-data-structures/graph-search
mode: harness
---
A **graph** is a set of **vertices** joined by **edges**: cities and roads, people and friendships, web pages and links. Here the graph is **undirected** (an edge works both ways) and vertices are numbered `0 .. n-1`.

## Adjacency list

For each vertex keep a linked list of its neighbours:

```c
typedef struct Edge {
    int to;
    struct Edge *next;
} Edge;

typedef struct {
    int n;
    Edge **adj;     // adj[v] is the first edge out of v
} Graph;
```

Adding the undirected edge `u - v` adds *two* list nodes: `v` to `adj[u]` and `u` to `adj[v]`. In this lesson new edges are **appended at the end** of the list, so neighbours are always visited in the order the edges were added.

## Breadth-first search

BFS explores in rings: first the start vertex, then everything one edge away, then two edges away, and so on. It uses a **queue** (an array with `head` and `tail` indexes is enough, since each vertex enters at most once). Because rings are visited in order, the first time BFS reaches a vertex is along a **shortest path** (fewest edges).

```c
dist[all] = -1;
dist[src] = 0;  enqueue(src);
while (queue not empty) {
    int u = dequeue();
    for (Edge *e = g->adj[u]; e; e = e->next)
        if (dist[e->to] == -1) {
            dist[e->to] = dist[u] + 1;
            enqueue(e->to);
        }
}
```

`dist[v] == -1` doubles as the "visited" marker.

## Depth-first search

DFS goes as deep as it can before backing up: visit a vertex, then recursively visit each unvisited neighbour in turn. You need a `visited` array, or the search loops forever in a cycle.

```c
static void dfs(const Graph *g, int u, char *seen, int *out, int *count) {
    seen[u] = 1;
    out[(*count)++] = u;
    for (Edge *e = g->adj[u]; e; e = e->next)
        if (!seen[e->to]) dfs(g, e->to, seen, out, count);
}
```

## Connected components

A **component** is a maximal group of vertices that can reach each other. Count them by looping over all vertices: each time you meet an unvisited one, that is a new component; run a DFS (or BFS) from it to mark the whole group.

## Common mistakes

- Adding only one direction of an undirected edge.
- Forgetting the visited check, causing endless recursion on cycles.
- Marking a vertex visited when it is *dequeued* instead of when it is *enqueued* (it then enters the queue many times).
- Not validating vertex numbers before indexing `adj`.

`g_new` and `g_free` are provided; implement the rest.
