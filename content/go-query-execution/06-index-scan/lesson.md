---
title: Index Range Scan
summary: Fetch only the rows in a key range by binary-searching a sorted index, and see where an index scan disagrees with a table scan around NULLs.
minutes: 50
difficulty: hard
tags: index, binary-search, range-scan, null
id: go-query-execution/index-scan
mode: harness
---
A table scan reads every row to answer `WHERE k BETWEEN 20 AND 40`. A sorted **index** of `(key, row id)` pairs lets you find the first entry at or above 20 by binary search and then walk forward until the keys exceed 40, fetching only the rows that qualify. The counters show it: `RowsRead` equals the number of matches, and the number of key comparisons grows like `log2(n)`, not `n`.

## Worked example

```text
table rows (id: key)       index (key, rowID), NULL skipped
0: 30                      (10, 1)
1: 10                      (20, 3)
2: NULL                    (20, 4)
3: 20                      (30, 0)
4: 20                      (40, 5)
5: 40
```

`WHERE k >= 20 AND k < 40`: binary-search the first entry with key >= 20, which is position 1. Walk: `(20,3)`, `(20,4)`, `(30,0)` qualify; `(40,5)` has key == hi with an exclusive bound, so stop. Rows fetched: 3. The other rows were never touched.

## Binary search with `sort.Search`

```go
// first position whose key is >= lo
i := sort.Search(len(idx.Entries), func(i int) bool {
    return idx.Cmp(idx.Entries[i].Key, lo.Val) >= 0
})
```

For an exclusive lower bound the predicate is `> 0` instead, so *every* duplicate of that key is skipped.

## Details that bite

- **Bounds** may be inclusive or exclusive, and either may be missing (`nil` means unbounded).
- **Duplicates**: equal keys are ordered by row id, so equality lookups return rows in table order. A stable sort over row-ordered input already gives that.
- **NULL is not indexed.** An index scan with no bounds therefore returns only non-NULL rows. A table scan with no predicate returns all rows. A planner may only substitute an index for a predicate that rejects NULL, such as `k >= 20`, never for a bare scan.
- Comparisons go through `Index.Cmp`, which counts them. The test relies on that, so bypassing it would defeat the check (a limitation of counting by convention, not by instrumentation).
- Once the walk has stopped, `Next` must not compare again; remember that you are done.
- Fetch the row and count `RowsRead` only after the key passed the upper-bound check.

Building the index is part of the task, but its cost is not measured here.
