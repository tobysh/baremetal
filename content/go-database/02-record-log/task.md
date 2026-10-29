Write two functions around this record format: `keyLen` (4 bytes, big-endian), `valLen` (4 bytes, big-endian), then the key bytes, then the value bytes.

- `AppendRecord(w io.Writer, key, value []byte) error` writes one record using a **single** call to `w.Write`. It returns an error if the key or the value is longer than `MaxField` (1 MiB), and writes nothing in that case. Return any error from `Write`.
- `ReadRecord(r io.Reader) (key, value []byte, err error)` reads one record.
  - If the stream is already at its end, return `io.EOF`.
  - If the stream ends part-way through a record (in the header or the body), return `io.ErrUnexpectedEOF`.
  - If a length field is larger than `MaxField`, return a non-nil error (not `io.EOF`) without allocating that much.
  - Empty keys and values are allowed, and must come back as zero-length slices.
