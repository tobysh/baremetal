Implement in `main.go`:

- `NewSort(in, keys []SortKey)`: reads all of `in` on the first `Next`, then returns rows ordered by `keys`; stable; NULL smallest (first ascending, last descending). `Open` reads nothing.
- `NewMergeJoin(left, right, lcol, rcol)`: both inputs are already sorted ascending on their join column with NULLs first. Output `left ++ right` as new rows in left order, right order within a left row. NULL keys never match. Read each input at most once, open each once, and stop reading as soon as one side can no longer match.
