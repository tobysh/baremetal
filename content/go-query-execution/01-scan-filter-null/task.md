Implement, in `main.go`:

- `And`, `Or`, `Not` over `Tri` (`False < Unknown < True`).
- `Compare(op, a, b)` for `=`, `!=`, `<`, `<=`, `>`, `>=` on `int64` or `string` values. Any NULL operand gives `Unknown`.
- `NewScan(t, st)`: an `Iterator` over `t.Rows` in order. `Open` restarts at the first row and adds 1 to `st.Opens`; each row returned adds 1 to `st.RowsRead`. After the end, `Next` keeps returning `nil, false`.
- `NewFilter(in, pred)`: returns only rows where `pred` is `True`. It must be lazy: one `Next` pulls only as many input rows as needed. `Open` and `Close` pass through to `in`.

The tests check results against an independent reference on random tables with NULLs, and check `RowsRead`, never time.
