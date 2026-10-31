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

Details that bite:

- **Bounds** may be inclusive or exclusive, and either may be missing. An exclusive lower bound must skip *every* duplicate of that key, so search for the first entry strictly greater.
- **Duplicates**: equal keys are ordered by row id, so equality lookups return rows in table order.
- **NULL is not indexed.** An index scan with no bounds therefore returns only non-NULL rows. A table scan with no predicate returns all rows. A planner may only substitute an index for a predicate that rejects NULL, such as `k >= 20`, never for a bare scan.
- Comparisons go through `Index.Cmp`, which counts them. The test relies on that, so bypassing it would defeat the check (a limitation of counting by convention, not by instrumentation).

Building the index is part of the task, but its cost is not measured here.
