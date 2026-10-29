Write `frame(payload []byte) []byte` and `unframe(data []byte) (payload []byte, n int, ok bool)` first. `ok` is false for both short and corrupt frames, since `Open` treats them the same.
===
Write `encodeOp(op byte, key, value string) []byte` for the payload, and `apply(op, payload)` that updates `db.m`. `Open` and `Put` can then share `apply`.
===
In `Open`: loop, `p, n, ok := unframe(log[pos:])`; stop if `!ok`; decode the payload (check `len(p) >= 5` and that `keyLen` fits) and stop on nonsense; apply; `pos += n`. At the end `db.log = append([]byte(nil), log[:pos]...)`.
===
`Compact`: collect and sort the keys, then `db.log = nil` and append `frame(encodeOp(1, k, db.m[k]))` for each.
