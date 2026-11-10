Implement `NewNestedLoopJoin(outer, inner Iterator, pred func(l, r Row) Tri) Iterator`.

- Output rows are `l` followed by `r` in a **new** slice, for each pair with `pred(l, r) == True`.
- Order: outer order, then inner order.
- `Open` opens the outer only. The inner is opened when an outer row has been read and closed when that row's scan ends, so an empty outer never touches the inner.
- Lazy: one `Next` reads only as many rows as it needs.
- `Close` closes whatever is open.

`EqCols` and `concatRows` are provided.

Edge cases the tests try: NULL keys on either side, heavy duplicates, an empty outer or inner input, non-equality and always-true predicates, and stopping early (the counters must show no extra reads). Output rows must not alias the inputs.
