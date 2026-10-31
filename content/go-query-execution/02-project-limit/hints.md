`Add`: evaluate both expressions, return `nil` if either result is `nil`, otherwise type-assert both to `int64`.
===
`Project.Next`: pull one row; if there is none return `nil, false`; otherwise allocate `make(Row, len(exprs))` and fill it.
===
Limit keeps `given` (rows returned) and `skipped` (whether the offset was consumed). Check `given >= n` before anything else so that `n == 0` reads nothing.
===
Skip the offset lazily on the first `Next`, not in `Open`. When the child runs out, set `given = n` so later calls do not pull again. Reset both fields in `Open`.
