`Entry` is given. Implement the table format from the lesson.

- `WriteTable(w io.Writer, entries []Entry) error` writes entries, then the index, then the footer. Return an error, and write nothing, if the keys are not strictly ascending. A tombstone entry stores flag 1 and no value bytes. An empty slice is valid and gives a table with just a footer.
- `OpenTable(r io.ReaderAt, size int64) (*Table, error)` reads the footer and the index (two `ReadAt` calls) and keeps the index in memory. Return an error for a table that is too small, has the wrong magic, or an inconsistent index. Never panic on damaged data.
- `(*Table) Get(key string) (Entry, bool, error)` finds the key by binary search in the in-memory index and then reads the entry with **exactly one** `ReadAt`. A key that is not in the index costs **no** reads. A tombstone is returned as an entry with `Delete` true and `true` for found.
- `(*Table) Len() int` is the number of entries.
