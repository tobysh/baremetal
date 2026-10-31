Implement `Scan(text string, baseline map[string]bool) []Finding` (type, `Preview` and `Fingerprint` are supplied). Return an empty or nil slice for clean text.

`main` scans a public configuration sample made from invented values. The hidden tests use fixtures with known line and column positions, near misses (23 or 25 characters, wrong prefixes, placeholders), several secrets per line, CRLF input, a baseline, and check that no finding field contains more of a secret than the supplied preview.
