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

```
table = map key -> rows of right
for l in left: for r in table[l.key]: emit l ++ r
```

Work drops from `|left| x |right|` reads to `|left| + |right|`, at the price of memory for the build side and no help for non-equality predicates.

Things to get right:

- **Build on the first `Next`**, not in `Open`, so an unused join costs nothing. The build side is read to the end even if the probe side turns out to be empty.
- **NULL keys never enter the table and never probe it.**
- **Duplicates:** keep every build row under its key, in arrival order, so the output order matches the nested loop exactly. That makes the tests' brute-force nested loop a valid reference.
- **Type matters.** `int64(1)` and `"1"` are different keys. If you build string keys with `fmt.Sprint` they collide. A `map[Value][]Row` keeps them apart.

*Honesty note:* the counters prove one pass per side. They do not prove the hash table is fast; that is a property of Go's map, not of your code.

The track challenge extends this join to a left outer join.
