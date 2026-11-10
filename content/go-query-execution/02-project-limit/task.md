Implement in `main.go`:

- `Add(a, b Expr) Expr` (int64 sum) and `Concat(a, b Expr) Expr` (string join). Either operand NULL gives NULL.
- `NewProject(in, exprs)`: for each input row, a fresh `Row` holding each expression's value. `Open`/`Close` pass through.
- `NewLimit(in, offset, n)`: discard `offset` rows, then return at most `n`. Once `n` rows were returned, or the input ended, never call the child's `Next` again. `n == 0` must read nothing. `Open` restarts the counting.

Edge cases the tests try: NULL in either operand, `offset` larger than the input, `n == 0`, `Next` called again after the limit is reached or the input ended (the child must not be touched), and the same pipeline reopened. Output rows must not share memory with input rows.
