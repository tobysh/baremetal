Implement in `main.go`:

- `BuildIndex(t, col) *Index`: one `Entry{key, rowID}` per row with a non-NULL key, sorted by key then row id.
- `NewIndexScan(t, idx, lo, hi, st)`: rows with `lo <= key <= hi` (each bound inclusive or exclusive; `nil` means unbounded) in index order. `Open` does the binary search and adds 1 to `st.Opens`; `Next` returns `t.Rows[rowID]` and adds 1 to `st.RowsRead` per row returned.

Budget enforced by the tests: at most `ceil(log2(entries+1)) + 1` comparisons to find the start, plus one per row walked, plus one to stop. All comparisons must use `idx.Cmp`. Once the range ends, `Next` must not compare again.
