`WriteTable`: build everything into a `bytes.Buffer` (data first, remembering `offset` and `length` of each entry as you go), then write it with one `w.Write`. Checking the order up front means a bad input writes nothing.
===
Footer: `binary.BigEndian.AppendUint64` three times: index offset, count, magic. The footer is the last 24 bytes of the output.
===
`OpenTable`: if `size < 24` return an error. `ReadAt(footer, size-24)`, tolerate `io.EOF` when `n == 24`. Check the magic and that `indexOffset <= size-24`. Read `size-24-indexOffset` bytes at `indexOffset` and parse exactly `count` index entries from them, checking lengths at each step.
===
Index entries in memory: `type indexEntry struct { key string; off int64; length uint32 }`. Check `off <= indexOffset`, then compare `length <= indexOffset - off` to avoid unsigned overflow. Track the expected next offset (initially zero), require contiguous entries, and reject nonascending keys. The final expected offset must equal `indexOffset`.
===
`Get`: `i := sort.Search(len(t.index), func(i int) bool { return t.index[i].key >= key })`. If `i == len(t.index)` or the key differs, return `Entry{}, false, nil` without touching the reader. Otherwise `ReadAt` a buffer of `length` bytes and decode the entry.
