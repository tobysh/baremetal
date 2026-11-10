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

```text
for l in outer:
    for r in inner:
        if pred(l, r) is True: emit l ++ r
```

In the iterator model that means `Open`-ing the inner child again for every outer row, which is exactly the cost you will see in the counters: 9 outer rows and 6 inner rows read 9 + 9 x 6 rows. It works for any predicate, not just equality, and needs no memory, which is why it stays in every planner's toolbox.

## Worked example

```text
users (outer)        orders (inner)        users JOIN orders ON id = uid
(1, ada)             (2, pen)              (1, ada, 1, ink)
(2, bo)              (1, ink)              (2, bo, 2, pen)
(3, cy)              (2, pad)              (2, bo, 2, pad)
                     (NULL, ghost)         cy has no match, ghost matches nobody
```

The inner scan is opened 3 times and reads 4 rows each time: 12 inner reads in total, plus 3 outer reads.

## Why `Next` needs state

A join returns one pair per call, but one outer row may produce many pairs, or none. So the operator remembers where it is:

```go
type nlj struct {
    outer, inner Iterator
    cur          Row   // current outer row; nil = need a new one
}
```

`Next` loops: no current row, pull one and open the inner; read the inner until a pair matches and return it (the inner iterator keeps its own position for the next call); when the inner ends, close it, set `cur = nil` and go round again.

## Edge cases that return in every join lesson

- **NULL keys never match**, not even each other: `NULL = NULL` is Unknown and a join keeps only True.
- **Duplicate keys multiply**: three left rows and three right rows with key 1 give nine results.
- **Empty outer**: the inner must never be opened.
- **Early stop**: if the caller closes the join mid-scan, the open inner scan must be closed too.

The result order is part of the contract: outer order first, inner order within each outer row. Laziness is too: you must not read more rows than the results pulled so far require. Build each result in a **new** slice (`concatRows`); appending to the outer row can silently overwrite shared memory.

*Limits:* this lesson covers inner joins only; outer joins come back as the track challenge.
