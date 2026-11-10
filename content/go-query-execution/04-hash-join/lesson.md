---
title: Hash Join
summary: Build a hash table once instead of rescanning the inner side, keeping NULL keys, duplicate keys and mixed key types correct in one pass per side.
minutes: 45
difficulty: hard
tags: join, hash, null, duplicates
id: go-query-execution/hash-join
mode: harness
---
The nested loop reads the inner side once per outer row. For an equality join there is a better way: read one side **once** into a hash table keyed by the join column (the *build* side), then stream the other (the *probe* side) and look each key up.

```text
table = map key -> rows of right
for l in left: for r in table[l.key]: emit l ++ r
```

Work drops from `|left| x |right|` reads to `|left| + |right|`, at the price of memory for the build side and no help for non-equality predicates.

## Worked example

```text
build (right = orders)       hash table                 probe (left = users), in order
(2, pen)                     1 -> [(1, ink)]            (1, ada) -> (1, ada, 1, ink)
(1, ink)                     2 -> [(2, pen), (2, pad)]  (2, bo)  -> (2, bo, 2, pen), (2, bo, 2, pad)
(2, pad)                     (NULL row not stored)      (3, cy)  -> no entry, nothing
(NULL, ghost)
```

Each input is opened once and each row read once: 3 + 4 = 7 reads, against 3 + 3 x 4 = 15 for the nested loop.

## The shape of `Next`

```go
type hashJoin struct {
    left, right Iterator
    table       map[Value][]Row
    built       bool
    cur         Row     // current probe row
    matches     []Row   // its matches
    i           int     // next match to emit
}
```

`Next` builds on first use, then loops: if `i < len(matches)` emit the next pair; otherwise pull a left row, look up `table[key]` and reset `i`.

## Things to get right

- **Build on the first `Next`**, not in `Open`, so an unused join costs nothing. The build side is read to the end even if the probe side turns out to be empty.
- **NULL keys never enter the table and never probe it.**
- **Duplicates:** keep every build row under its key, in arrival order, so the output order matches the nested loop exactly. That makes the tests' brute-force nested loop a valid reference.
- **Type matters.** `int64(1)` and `"1"` are different keys. If you build string keys with `fmt.Sprint` they collide. A `map[Value][]Row` keeps them apart.
- **Reopening:** `Open` after `Close` must clear the table, the built flag and the cursor.

*Honesty note:* the counters prove one pass per side. They do not prove the hash table is fast; that is a property of Go's map, not of your code.

The track challenge extends this join to a left outer join.
