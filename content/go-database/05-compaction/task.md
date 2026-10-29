`Entry`, `EncodeEntries` and `DecodeEntries` are given in the starter.

- `Compact(entries []Entry, dropTombstones bool) []Entry` takes entries in the order they were written (oldest first) and returns one entry per key: the **newest** one. The result is sorted by key, ascending. When `dropTombstones` is true, keys whose newest entry is a tombstone are removed from the result completely; when false, the tombstone entry is kept. The input must not be modified. An empty input gives an empty, non-nil slice.
- `CompactLog(log []byte) ([]byte, error)` decodes a log with `DecodeEntries`, compacts it fully (tombstones dropped) and encodes the result. Return the decoding error if the log is damaged.
