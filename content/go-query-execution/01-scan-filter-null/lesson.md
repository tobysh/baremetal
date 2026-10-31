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

## Three-valued logic

SQL has a third truth value. Comparing anything with NULL, including `NULL = NULL`, gives **Unknown**:

| a | b | a AND b | a OR b |
|---|---|---|---|
| True | Unknown | Unknown | True |
| False | Unknown | False | Unknown |
| Unknown | Unknown | Unknown | Unknown |

`NOT Unknown` is Unknown. A filter keeps a row only when the predicate is exactly **True**, so `WHERE NOT (score > 50)` drops a row whose score is NULL, as does `WHERE score > 50`. Code that turns the result into a Go `bool` too early gets this wrong.

Run the starter's example once your functions work: the NULL score row is read but not returned.

*Limits of this model:* values are `nil`, `int64` or `string`; a column never mixes types, and comparing mixed types is treated as Unknown.
