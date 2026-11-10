Loop over records with `off += 32` and a bound written as `n - off >= 32`. That stops cleanly for a partial last record and cannot overflow.
===
Handle the special first byte before anything else: `0x00` means `break`, `0xe5` means "deleted, but keep going". Then skip any record whose attribute is `0x0f` or has bit `0x08`.
===
Build the name in two steps: find the length of each part by moving an index backwards past spaces, then copy. Add `.` only when the extension length is nonzero, and substitute `_` for the first character of a deleted entry.
===
`cluster` is `(uint32_t)hi << 16 | lo`, with `hi` read from offset 20 and `lo` from 26. For the size and the two words write one small little-endian helper and reuse it.
