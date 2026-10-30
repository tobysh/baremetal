Start with `if len(pub) != ed25519.PublicKeySize || !ed25519.Verify(pub, manifest, sig) { return ErrBadSignature }`. `Verify` already returns false for a signature of the wrong length.
===
For parsing, `bytes.HasSuffix(manifest, []byte("\n"))` first, then `strings.Split(strings.TrimSuffix(text, "\n"), "\n")`. An empty element means a blank line, which is an error. Use `strings.Split(line, " ")` and require exactly 4 fields for a file line, so double spaces fail.
===
Canonical size: `strconv.ParseUint(s, 10, 63)` succeeds on `007` too, so also require `strconv.FormatUint(n, 10) == s`. Reuse the digest rules from lesson 1 for the `sha256:` field, lower case only.
===
Parse completely into a slice of entries, and a `seen` set for duplicates, before touching `files`. Then walk the entries in order for missing/mismatch and finally walk `files` (the map, so sort the keys to be deterministic if you report the first extra one) for unlisted names.
