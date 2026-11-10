---
title: Project and Limit
summary: Compute output columns with NULL-propagating expressions, then stop a pipeline early by refusing to pull more rows than a LIMIT needs.
minutes: 35
difficulty: medium
tags: iterator, projection, limit, laziness
id: go-query-execution/project-limit
mode: harness
---
**Project** turns each input row into a new row by evaluating one expression per output column. An expression is just a function from the input row to a value, so `SELECT id, price + tax` is two expressions: `Col(0)` and `Add(Col(1), Col(2))`. Expressions follow the NULL rule from the previous lesson: `NULL + 1` is NULL, and so is NULL concatenated with text.

```go
type Expr func(Row) Value

// price + tax, NULL if either is NULL
func Add(a, b Expr) Expr {
    return func(r Row) Value {
        x, y := a(r), b(r)
        if x == nil || y == nil { return nil }
        return x.(int64) + y.(int64)
    }
}
```

Project must build a *new* `Row` each time. If it returned a slice of the input row, a caller editing a result would edit the table.

**Limit** shows why the pull model matters. `LIMIT 3` should read three rows of a million-row table, not a million. Because nothing runs until the parent calls `Next`, Limit only has to stop calling.

```text
LIMIT 2 OFFSET 1 over a scan of 5 rows
Next #1: pull row1 (skipped), pull row2 -> return    RowsRead = 2
Next #2: pull row3 -> return                         RowsRead = 3
Next #3: already gave 2 -> return false              RowsRead = 3  (rows 4 and 5 untouched)
```

## Pitfalls

- After Limit has returned its last row, a further `Next` must not touch the child. One extra pull reads (and pays for) a row nobody asked for. Check the counter *before* pulling.
- `OFFSET` rows are pulled and thrown away, so they do count as work. Skip them lazily on the first `Next`, not in `Open`, so an unused Limit costs nothing.
- `LIMIT 0` must read nothing at all.
- If the child runs out early, remember it: do not ask again on the next call.
- `Open` after `Close` restarts the counting; reset your fields there.

The scan and a simple filter are provided. The tests check the exact number of rows read through a Filter, Project and Limit pipeline, not just the output.
