---
title: Nested-Loop Join
summary: Combine two inputs with the simplest join, an outer loop over an inner re-scan, and see exactly how many rows that costs.
minutes: 40
difficulty: medium
tags: join, nested-loop, null, duplicates
id: go-query-execution/nested-loop-join
mode: harness
---
A join pairs rows from two inputs that satisfy a predicate. The **nested-loop join** is the definition turned into code: for every row of the outer input, scan the whole inner input and emit each pair that matches.

```
for l in outer:
    for r in inner:
        if pred(l, r) is True: emit l ++ r
```

In the iterator model that means `Open`-ing the inner child again for every outer row, which is exactly the cost you will see in the counters: 9 outer rows and 6 inner rows read 9 + 9 x 6 rows. It works for any predicate, not just equality, and needs no memory, which is why it stays in every planner's toolbox.

Two edge cases appear here for the first time and return in every join lesson:

- **NULL keys never match**, not even each other: `NULL = NULL` is Unknown and a join keeps only True.
- **Duplicate keys multiply**: three left rows and three right rows with key 1 give nine results.

The result order is part of the contract: outer order first, inner order within each outer row. Laziness is too: you must not read more rows than the results pulled so far require.

*Limits:* this lesson covers inner joins only; outer joins come back as the track challenge.
