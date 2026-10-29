Use `mem map[string]Entry` and `tables []*table` with the newest table at index 0, so `Get` just ranges over `tables` in order.
===
`Put`: `db.mem[key] = Entry{Key: key, Value: copyOfValue}`. `Delete`: `db.mem[key] = Entry{Key: key, Delete: true}`. After either, `if len(db.mem) >= db.limit { db.Flush() }`.
===
`Flush`: return early if `len(db.mem) == 0`. Collect the map's entries into a slice, `sort.Slice` it by `Key`, call `newTable`, then `db.tables = append([]*table{t}, db.tables...)` and `db.mem = map[string]Entry{}`.
===
`Get`: check `db.mem` first. For each table: `e, ok := t.get(key)`; if `ok`, return `copy of e.Value, !e.Delete`. When `e.Delete` is true return `nil, false` immediately, do not continue.
