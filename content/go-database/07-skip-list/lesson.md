---
title: A Skip List
summary: Keep keys in sorted order with fast inserts and lookups, using a linked list with express lanes.
minutes: 45
difficulty: hard
tags: skip-list, sorted, data-structures
id: go-database/skip-list
mode: harness
---
A map is fast but unordered. Databases need **sorted** data: to scan a range, to write sorted files, to find "the first key at or after X". You could keep a sorted slice, but inserting into the middle moves everything after it. A linked list inserts cheaply but searching it means walking from the start.

A **skip list** is a sorted linked list with express lanes. Every node is in the bottom list (level 0). About half the nodes also appear in a second list that skips ahead; about a quarter in a third, and so on. To search, start at the top lane, ride it as far as you can without passing the key, drop down a lane, and repeat. That takes about `log2(n)` steps. Skip lists are simple, and are the classic in-memory table of write-optimised databases (LevelDB and RocksDB use one).

```
level 2:  head ───────────────► 30 ───────────────────────► nil
level 1:  head ──────► 10 ────► 30 ───────► 50 ────────────► nil
level 0:  head ► 5 ► 10 ► 20 ► 30 ► 40 ► 50 ► 60 ► nil
```

## The node

Each node has a key, a value, and a `next` slice with one pointer per level it takes part in. `next[0]` is the bottom list.

```go
type node struct {
	key   string
	value []byte
	next  []*node
}
```

The `head` is a dummy node that has all `maxHeight` levels and holds no data.

## Coin flips

A new node gets a random height: start at 1 and keep adding a level while a coin flip says "heads" (and the height is below `maxHeight`). Half the nodes stay at height 1, a quarter reach 2, and so on, which builds the lanes without any rebalancing code. The starter gives you a seeded `*rand.Rand` so that tests are repeatable.

## Finding the place

Insert and delete both need the **update path**: for each level, the last node whose key is smaller than the target. Walk from the top level down:

```go
x := s.head
for lvl := s.height - 1; lvl >= 0; lvl-- {
	for x.next[lvl] != nil && x.next[lvl].key < key {
		x = x.next[lvl]
	}
	update[lvl] = x // the node just before the target on this level
}
```

After the loop, `update[0].next[0]` is the first node with key >= `key`: the exact match if it is there, and the answer to `Seek` either way.

To insert, splice the new node in behind each `update[lvl]` for the levels it joins (if its height is above the list's current height, the head is the predecessor for the extra levels). To delete, make each `update[lvl].next[lvl]` skip over the node, and lower the list's height while the top level is empty.
