`BuildIndex`: loop over rows, skip `nil` keys, append entries, then `sort.SliceStable` using `ix.Cmp`. Stable sort over row-ordered input already orders duplicates by row id.
===
`sort.Search(n, f)` finds the first index where `f` is true. For an inclusive lower bound use `Cmp(key, lo) >= 0`; for an exclusive one use `> 0`.
===
`Next`: stop if the position is past the end; otherwise compare the entry's key with `hi`. Stop (and remember you stopped) if it is greater, or equal with an exclusive bound.
===
Only after the key passes the check should you fetch the row and increment `RowsRead`.
