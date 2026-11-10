---
title: Scan, Filter and NULL Logic
summary: Pull rows through an iterator pipeline and learn why a WHERE clause keeps only rows whose condition is True, never Unknown.
minutes: 40
difficulty: medium
tags: iterator, null, three-valued-logic
id: go-query-execution/scan-filter-null
mode: harness
---
A query executor is a tree of **operators**. Each one implements the same small interface, `Open`, `Next`, `Close`, and asks its child for one row at a time. Nothing is computed until the root pulls, so a filter that finds its answer early leaves the rest of the table unread. This is the *iterator* (Volcano) model. Your storage lessons stopped at bytes and keys; here the question is how rows flow.

## Pulling rows through a pipeline

For `SELECT * FROM t WHERE score > 50` the tree is two nodes, Filter on top of Scan:

```text
Collect (the caller)
  |  Next()  ->  row (3, 90)
Filter  score > 50
  |  Next()  ->  (1, 40)  rejected, ask again
  |  Next()  ->  (2, NULL) rejected, ask again
  |  Next()  ->  (3, 90)  accepted
Scan t   (RowsRead = 3)
```

Filter is a loop around its child's `Next`. The scan only counts a row when it hands it out, so the counter tells you exactly how much work was pulled.

```go
func (f *filter) Next() (Row, bool) {
    for {
        r, ok := f.in.Next()
        if !ok { return nil, false }
        if keep(r) { return r, true }
    }
}
```

Every operator in this track is a variation on that shape: a struct holding its children and a little state, a `Next` that pulls from them.

## Three-valued logic

SQL has a third truth value. Comparing anything with NULL, including `NULL = NULL`, gives **Unknown**:

| a | b | a AND b | a OR b |
|---|---|---|---|
| True | Unknown | Unknown | True |
| False | Unknown | False | Unknown |
| Unknown | Unknown | Unknown | Unknown |

`NOT Unknown` is Unknown. A filter keeps a row only when the predicate is exactly **True**, so `WHERE NOT (score > 50)` drops a row whose score is NULL, as does `WHERE score > 50`. Code that turns the result into a Go `bool` too early gets this wrong.

## Pitfalls

- With `False=0, Unknown=1, True=2`, `And` is the minimum and `Or` the maximum; `Not` maps `0 -> 2`, `2 -> 0` and leaves `1` alone.
- Test for `nil` **before** type-switching in `Compare`; a nil interface matches no type case.
- Keep only `True`. `pred(row) != False` would let Unknown through.
- A scan that is exhausted must stay exhausted and must not count extra reads.
- Never modify a row you received; the table owns it.

*Limits of this model:* values are `nil`, `int64` or `string`; a column never mixes types, and comparing mixed types is treated as Unknown.
