Implement three functions (`ErrDigestFormat`, `ErrDigestMismatch` and `ErrTooLarge` are supplied):

- `ParseDigest(s string) ([32]byte, error)` accepts exactly `sha256:` followed by 64 lowercase hex characters, otherwise `ErrDigestFormat`.
- `VerifyArtifact(data []byte, want string) error` parses `want` first (a bad `want` is `ErrDigestFormat` no matter what `data` is), then returns `ErrDigestMismatch` unless SHA-256 of `data` equals it.
- `VerifyReader(r io.Reader, maxSize int64, want string) error` does the same for a stream. Parse `want` before reading anything. If the stream holds more than `maxSize` bytes, return `ErrTooLarge`; exactly `maxSize` is fine. A read error other than EOF is returned (wrapped is fine, it must satisfy `errors.Is`). Hash while reading; do not buffer the whole stream.

`main` verifies a small public artifact. The hidden tests use other data, split reads that deliver one byte at a time, oversize streams, failing readers, and a spread of malformed expectations. SHA-256 of the three bytes `abc` is `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad`.
