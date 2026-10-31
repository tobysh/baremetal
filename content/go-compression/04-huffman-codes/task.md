Implement `BuildLengths` and `CanonicalCodes` in `starter.go`.

- `BuildLengths(freq)` returns a slice the same size as `freq` with each used symbol's code length (0 for unused). Use the tie-break from the lesson. No symbols: all zeros. One symbol: length 1.
- `CanonicalCodes(lengths)` returns a `Code{Bits, Len}` per symbol (`Len` 0 for unused). Return `ErrBadLengths` for a negative length, a length above `MaxCodeLen`, or an oversubscribed set. Incomplete sets (like a single length-1 symbol) are allowed.

Hidden checks: exact lengths for fixed frequency tables computed independently, the RFC 1951 example, optimal cost, Kraft sum exactly 1, prefix-freeness, and tie cases (`[1 1 1 1 1]` must give `[3 3 2 2 2]`).