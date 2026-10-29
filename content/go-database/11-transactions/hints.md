`DB` has `data map[string]string` and `log []byte`. `Tx` has `db *DB`, `ops []txOp` and `done bool`, where `txOp` is `{del bool; key, value string}`.
===
`Tx.Get`: loop `i` from `len(ops)-1` down to 0; the first op on the key decides (a delete means `"", false`). If none matches, `return tx.db.Get(key)`.
===
Write `encodeBatch(ops []txOp) []byte` and `decodeBatch(p []byte) ([]txOp, bool)`. In `decodeBatch` check every length against what remains (in `uint64`) and return `false` for anything that does not parse to exactly the end of the payload.
===
`Commit`: if `done` return `ErrTxDone`; set `done = true`; if there are no ops return nil; `db.log = append(db.log, frame(encodeBatch(ops))...)`; then `db.apply(ops)`.
===
`Recover`: loop `p, n, ok := unframe(log[pos:])`; stop if `!ok`; `ops, ok := decodeBatch(p)`; stop if `!ok`; `db.apply(ops)`; `pos += n`. Finish with `db.log = append([]byte(nil), log[:pos]...)`.
