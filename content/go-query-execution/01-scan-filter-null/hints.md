`And` is False if either side is False, otherwise Unknown if either is Unknown, otherwise True. `Or` is the mirror image. With `False=0, Unknown=1, True=2`, AND is the minimum and OR is the maximum.
===
In `Compare`, test for `nil` first and return `Unknown`. Then type-switch on `a`, assert `b` to the same type, and compute a sign `-1/0/1` once; the operator only looks at the sign.
===
`Next` of the scan: if `pos >= len(rows)` return `nil, false` without touching `RowsRead`; otherwise increment both `pos` and `RowsRead`.
===
`Filter.Next` is a loop: pull from `in`, return the row if `pred(row) == True`, otherwise pull again, and return `nil, false` when `in` is exhausted. Do not compare with `!= False`.
