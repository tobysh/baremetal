---
title: Grouping with NULL Values
summary: Group nullable keys, distinguish COUNT(*) from COUNT(value), and compute nullable sums.
minutes: 40
difficulty: medium
id: go-query-execution/group-aggregate
---
Aggregation collapses many rows into one row per **group**. `SELECT k, COUNT(*), COUNT(v), SUM(v) FROM t GROUP BY k` splits the input by the value of `k`, then folds each group into running totals. The operator is *blocking*: it cannot emit a group until it has seen every row, because a later row may still belong to it. Here it takes a whole slice, which is the same thing after the child has been drained.

## Two different NULL rules

NULL shows up in two places and SQL treats them differently:

- **NULL as a group key.** `NULL = NULL` is Unknown in a filter, but `GROUP BY` asks a different question, "are these two keys not distinct?", and the answer for two NULLs is yes. All NULL keys land in **one** group.
- **NULL as an aggregated value.** Aggregates skip NULLs. `COUNT(*)` counts rows, `COUNT(v)` counts rows whose `v` is not NULL, and `SUM(v)` adds only the non-NULL values. If a group has *no* non-NULL values, `SUM` is **NULL**, not 0. `SUM` of `5` and `-5` is a real, valid `0`.

## Worked example

```text
input (k, v)        output: k     COUNT(*)  COUNT(v)  SUM(v)
(a, 1)                      a     2         1         1
(a, NULL)                   NULL  2         1         5
(NULL, 5)                   b     1         0         NULL
(NULL, NULL)
(b, NULL)
```

Groups appear in order of first appearance: `a`, `NULL`, `b`. No sorting is needed, and the output is deterministic.

## The shape of the code

A slice keeps the order and a map finds the group quickly:

```go
index := map[Value]int{}   // key -> position in out
out := []Group{}
for _, r := range rows {
    i, seen := index[r.Key]
    if !seen {
        i = len(out)
        index[r.Key] = i
        out = append(out, Group{Key: r.Key})
    }
    // update out[i] ...
}
```

## Pitfalls

- A `Value` has two fields. `Value{N: 7, Valid: false}` and `Value{N: 0, Valid: false}` are different map keys even though both are NULL. Normalise a NULL key (set `N` to 0) **before** the lookup, and store the normalised key in the output.
- Do not confuse NULL with zero. The key `0` and a NULL key are different groups, and `Sum` must not become valid just because a row exists; it becomes valid only when a non-NULL value is added.
- `Rows` and `Count` differ exactly by the NULL values. Track both.
- Appending to the slice may move it, so take `&out[i]` only after the append, or index by position each time.

This model uses signed 64-bit integers and the inputs never overflow.
