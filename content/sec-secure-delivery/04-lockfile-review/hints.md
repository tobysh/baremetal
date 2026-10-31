Parse both files into `map[string]Pkg`, collect the union of names, sort it, and for each name decide which of added / removed / changed applies. Collect each package's findings in kind order, so the final order follows from iterating names in sorted order.
===
For a package in both files: compare `Version` with `cmpVersion`. Higher is `upgrade`, lower is `downgrade`, equal with different digest is `hash-changed`. Independently, a different `Source` is `source-changed`.
===
`source-not-allowed` applies to every package of the **new** lockfile (added or not) whose source is not allowed. Remember where it falls in the kind order.
===
Levenshtein distance 1 means: same length with exactly one differing byte, or lengths differing by one where deleting one byte of the longer gives the shorter. You do not need a full dynamic-programming table; write a small `oneEdit(a, b string) bool`.
