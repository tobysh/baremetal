Implement `Store`, which keeps all versions of every key.

- `NewStore() *Store`. Every `Put` or `Delete` increases an internal sequence counter, starting at 1 for the first write, and adds a version to the key. `Delete` on a missing key still records a tombstone version.
- `Get(key string) (string, bool)` reads the newest version (not found if it is a tombstone).
- `Snapshot() *Snapshot` records the current counter value and registers itself as live.
- `(*Snapshot) Get(key string) (string, bool)` reads the newest version with `seq <=` the snapshot's number. `(*Snapshot) Keys() []string` lists, in ascending order, every key that is alive at that point (empty, non-nil slice if none).
- `(*Snapshot) Close()` unregisters the snapshot. Closing twice is harmless.
- `(*Store) Versions() int` is the total number of versions kept across all keys.
- `(*Store) Prune()` discards versions that no live snapshot (nor the present) can need, following the rules in the lesson. Reads through snapshots and `Get` must give the same answers before and after.
