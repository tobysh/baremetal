For `Sort`, `sort.SliceStable` with a comparator that walks the keys: compare cell by cell, flip the sign for `Desc`, return at the first non-zero result. Write a `cmpValue` that treats `nil` as smaller than everything.
===
Do the reading lazily in `Next` (a `done` flag), not in `Open`.
===
Merge join: keep the current left row and current right row. If the left key is NULL, advance left. Otherwise advance right while its key is NULL or smaller than the left key. If the right key is larger, advance left.
===
On equal keys, read the whole run of equal right rows into a slice (the group). Emit left x group. When the next left row has the same key, replay the same group; otherwise drop it and carry on from the right row that ended the run.
