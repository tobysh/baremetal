Start with `merge`: two indices `i` and `j`, one output index `k`. Pick from `a` unless `b[j]` is strictly smaller.
===
For `merge_sort`: allocate `tmp` once, call a recursive helper `sort_range(a, tmp, n)` that returns if `n < 2`, sorts both halves, merges them into `tmp` and copies `tmp` back over `a` with `memcpy`. Free `tmp` at the end.
===
`merge_sort_recs` is the same shape with `Rec` instead of `int`, and the merge compares `.key`: take from the right half only if `b[j].key < a[i].key`. Since the right half's elements come later in the original order, taking from the left on ties keeps stability.
