Track `(line, col)` from `(1, 1)`, stopping before a character whose starting byte offset is at least `pos`.
===
Use `src.lines().nth(line - 1).unwrap_or("")` for the excerpt and the decimal line number length for the caret gutter. Runtime errors need no source scan.
