---
title: "Capstone: LRU Cache"
summary: Combine a hash table and a doubly linked list into a cache that evicts the least recently used entry, with every operation in constant time.
minutes: 60
difficulty: hard
tags: hash-table, linked-lists, caching, data-structures
id: c-data-structures/lru-cache
mode: harness
---
A **cache** keeps recent results close at hand so you do not recompute or refetch them. Memory is limited, so when it is full something must go. An **LRU** (least recently used) cache evicts the entry that has gone unused the longest, betting that what you touched lately you will touch again. Browsers, databases and operating systems all use this idea.

The requirement: `get`, `put` and eviction should all be O(1). No single structure does that:

- A **hash table** finds an entry by key in O(1), but knows nothing about recency.
- A **doubly linked list** can keep entries in recency order (most recent at the head, oldest at the tail) and can move or remove a node in O(1) *if you already hold a pointer to it*.

So use both on the same nodes. Each node sits in the list **and** in a hash chain:

```c
typedef struct LNode {
    int key, val;
    struct LNode *prev, *next;     // recency list
    struct LNode *hnext;           // hash chain
} LNode;

typedef struct {
    size_t cap, count;
    LNode *head, *tail;            // head = most recent, tail = least recent
    LNode **buckets;
    size_t nb;
} Lru;
```

The hash table gets you to the node; the list pointers let you reorder it without searching.

## Operations

- `get(key)`: find the node via the hash chain. If found, **move it to the head** (it is now the most recent) and return its value.
- `put(key, val)`: if the key exists, update the value and move the node to the head. Otherwise create a node at the head and add it to its hash chain. If `count` now exceeds `cap`, **evict the tail**: unlink it from the list *and* remove it from its hash chain, then free it.

Helper functions keep this manageable: `list_unlink`, `list_push_front`, `hash_find`, `hash_insert`, `hash_remove`. Write and think about each in isolation.

## The tricky part

Removing a node from a hash chain needs the chain's predecessor, so walk the chain from the bucket (chains are short). Every other step uses the stored pointers. Whenever you free a node, make sure it is out of **both** structures first, or a later lookup follows a dangling pointer.

A fixed bucket count is fine here (e.g. `nb = 2 * cap + 1`, at least 1), because the cache never holds more than `cap` entries.

## Common mistakes

- Moving a node to the front without first unlinking it (the list becomes a cycle).
- Evicting from the list but leaving the node in its hash chain.
- Updating `head` but not `tail` when the list had one node.
- Treating a `get` as read-only: in an LRU cache, reading refreshes recency.

This is the final lesson: it uses nearly everything before it.
