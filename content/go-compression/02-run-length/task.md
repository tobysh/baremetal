Implement `RLEEncode` and `RLEDecode` in `starter.go`.

- `RLEEncode(data)` follows the canonical rule from the lesson exactly (runs of 3+ become repeats of at most 129; the rest becomes literals of at most 128). The hidden tests compare against a reference built by a different route and against fixed byte vectors, so `aaabcc` must give `81 61 02 62 63 63`, and a run of 130 `a` must give a repeat of 129 and then a one-byte literal.
- `RLEDecode(enc)` returns the original bytes, or `ErrTruncated` if a token is cut short. Accept every valid stream, not just encoder output.

Hidden tests also cut every encoding at every length: each cut must either decode to a prefix of the original (at a token boundary) or return `ErrTruncated`.