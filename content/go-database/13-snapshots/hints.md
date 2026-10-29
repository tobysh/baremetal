Fields: `seq uint64`, `versions map[string][]version` (each slice in ascending `seq`), and `snaps map[*Snapshot]struct{}`. A `Snapshot` holds `store *Store` and `seq uint64`.
===
One helper does all reads: `visible(vs []version, seq uint64) (version, bool)` returns the last version with `v.seq <= seq`. `Store.Get` uses `s.seq`, `Snapshot.Get` uses its own `seq`.
===
`Snapshot.Keys`: loop over `store.versions`, call the helper for each key, include the key if a version is visible and it is not `deleted`. Sort the result.
===
`Prune`: `oldest := s.seq`; for every live snapshot, `oldest = min(oldest, snap.seq)`. For each key, find `i`, the index of the newest version with `seq <= oldest`; keep `vs[i:]`; if `vs[i].deleted` keep `vs[i+1:]` instead. If nothing remains, `delete(s.versions, key)`. Deleting map keys while ranging over the map is allowed in Go.
===
A key whose first version is newer than `oldest` has no such `i`; keep all of its versions.
