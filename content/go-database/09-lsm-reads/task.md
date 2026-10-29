`Entry`, the `table` type, `newTable` and the `tableReads` counter are given. A `table` is a sorted slice of entries with a binary-search `get`. Build `DB`:

- `NewDB(memLimit int) *DB` makes a database that flushes automatically as soon as its memtable holds `memLimit` or more entries (counting each key once, tombstones included).
- `Put(key string, value []byte)` and `Delete(key string)` write to the memtable (store a copy of the value), then flush if the limit is reached.
- `Flush()` sorts the memtable into a new `table`, puts it **in front of** the older ones and empties the memtable. It does nothing if the memtable is empty.
- `Get(key string) ([]byte, bool)` looks in the memtable, then in the tables from newest to oldest, and returns the first entry it finds. A tombstone means `nil, false`. It must not look at older tables once it has an answer.
- `TableCount() int` is the number of tables and `MemLen() int` the number of entries in the memtable.
