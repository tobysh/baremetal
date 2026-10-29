---
title: Hash Table with Chaining
summary: Hang a linked list off every bucket so collisions never run out of room. Learn string hashing, owning copies of keys, and resizing by rehashing.
minutes: 40
difficulty: medium
tags: hash-table, linked-lists, strings, data-structures
id: c-data-structures/hash-chaining
mode: harness
---
Open addressing has a hard limit: when the array is full, it is full. **Chaining** removes the limit. Each bucket holds a linked list of the entries that hash there, so a collision just makes a chain one node longer. You met this idea in [Systems Programming in C](/learn/c-systems) as a first hash table; here you make it complete.

```c
typedef struct Entry {
    char *key;
    int val;
    struct Entry *next;
} Entry;

typedef struct {
    Entry **buckets;   // array of nbuckets list heads
    size_t nbuckets;
    size_t count;
} StrMap;
```

## Hashing a string

A string hash mixes every character into one number. The classic **djb2**:

```c
unsigned long h = 5381;
for (const char *p = s; *p; p++)
    h = h * 33 + (unsigned char)*p;
```

The bucket index is `h % nbuckets`.

## Owning your keys

The caller's `const char *key` may point at a buffer that changes or disappears later. The map must store its **own copy**: `malloc(strlen(key) + 1)` then `memcpy`, and `free` it when the entry is removed. A map that keeps the caller's pointer works in simple tests and corrupts in real programs.

## Chain operations

- **Lookup**: compute the bucket, walk the chain, compare with `strcmp`.
- **Insert**: if the key exists, update its value; otherwise make a new entry and link it at the head of the chain.
- **Delete**: walk with a pointer to the *link* that points at the current entry (`Entry **link`), so removing the head and removing a middle node are the same code.

```c
Entry **link = &m->buckets[b];
while (*link && strcmp((*link)->key, key) != 0) link = &(*link)->next;
if (*link) { Entry *dead = *link; *link = dead->next; free(dead->key); free(dead); }
```

## Resizing

Long chains make lookups slow. Keep the **load factor** (`count / nbuckets`) low: after an insert that makes `count > 2 * nbuckets`, double the bucket count and **rehash**: move every entry into the new array by recomputing its bucket (the old index no longer applies). You can reuse the existing `Entry` nodes; just relink them.

## Common mistakes

- Storing the caller's key pointer instead of a copy.
- Forgetting to free the key as well as the entry.
- Not recomputing the bucket during a rehash.
- Counting the update of an existing key as a new entry.

Implement the map.
