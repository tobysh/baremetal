There are `len(writes)+1` cuts: cut 0 applies nothing and cut `len(writes)` applies everything. A loop `for k := 0; k <= len(writes); k++` visits them all.
===
At each `k`, first check the current state (`Ptr >= 0 && !Used[Ptr]` means unsafe, so record `k`), and only afterwards apply `writes[k]` if `k < len(writes)`. Checking after applying would shift every answer by one.
===
Work on a copy: `append([]bool(nil), initial.Used...)`. The tests compare the caller's bitmap before and after.
===
`"ptr"` writes set `Ptr = w.Value` and ignore `w.Block`; `"bit"` writes set `used[w.Block] = (w.Value == 1)`. Remember `Ptr == -1` is always safe.
