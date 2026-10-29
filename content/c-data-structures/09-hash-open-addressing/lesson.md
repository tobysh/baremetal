---
title: Hash Table with Open Addressing
summary: Store key-value pairs directly in a fixed array. Learn hashing, linear probing, and why deleting needs tombstones.
minutes: 45
difficulty: hard
tags: hash-table, arrays, data-structures
id: c-data-structures/hash-open-addressing
mode: harness
---
A **hash table** finds a value by key in (on average) constant time. The idea: a **hash function** turns a key into a number, and that number, reduced to the array size, says where to look.

```c
size_t slot = hash(key) & (cap - 1);   // cap is a power of two
```

(`& (cap - 1)` is a fast way to compute `% cap` when `cap` is a power of two.)

## Collisions and linear probing

Two different keys can land in the same slot: a **collision**. In **open addressing** the table is one array and a colliding key simply takes the next free slot, wrapping around at the end. This is **linear probing**:

```c
size_t i = hash(key) & (cap - 1);
while (slots[i].state == USED && slots[i].key != key)
    i = (i + 1) & (cap - 1);
```

To look a key up, follow the same probe sequence: the key is found when you meet it, and absent when you meet an **EMPTY** slot (an empty slot proves the key was never pushed past that point).

## The deletion problem

Suppose A and B collide: A sits in slot 3 and B was pushed to slot 4. If you delete A by marking slot 3 EMPTY, a search for B stops at slot 3 and reports "absent". Wrong!

The fix is a third state, **DELETED** (a *tombstone*). Lookups skip over tombstones and keep probing. Inserts may reuse the first tombstone they pass, but only *after* finishing the search to be sure the key is not further along.

```c
typedef enum { EMPTY, USED, DELETED } State;
typedef struct { int key, val; State state; } Slot;
```

## A full table

With a fixed capacity, a table can become completely full of `USED` and `DELETED` slots with no `EMPTY` one left. A lookup that waits for an empty slot would then loop forever. Bound every probe loop to at most `cap` steps.

## Common mistakes

- Treating a tombstone like an empty slot when searching.
- Inserting a duplicate key because you stopped at the first free slot instead of checking the rest of the chain.
- Counting tombstones in `count`. `count` is the number of live entries.

Implement the table. The hash function is provided.
