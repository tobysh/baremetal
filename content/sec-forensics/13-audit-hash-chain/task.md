Read standard input: at most 500 lines, each at most 300 characters. Line N (counting from 1) must be four fields separated by `|`: `SEQ|ACTOR|ACTION|HASH`.

- `SEQ` is a positive decimal number with no leading zero and only digits (at most 9 digits).
- `ACTOR` and `ACTION` are non-empty and contain no `|`; spaces are allowed.
- `HASH` is exactly 16 lowercase hex digits.
- Sequence numbers must run 1, 2, 3, ... in order.
- The expected hash of an entry is `%016llx` of FNV-1a 64 (offset basis 14695981039346656037, multiplier 1099511628211, XOR the byte then multiply, modulo 2^64) over the bytes of `PREV|SEQ|ACTOR|ACTION`, where `PREV` is the previous entry's `HASH` text, or `0000000000000000` for the first entry, and `SEQ` is written in plain decimal.

Check each line in this order and stop at the first failure, printing exactly one line:

1. not well formed (including a blank line, three or five fields, an empty field, a bad `SEQ` or a bad `HASH`): `BROKEN at line N: malformed`
2. wrong sequence number: `BROKEN at line N: sequence (expected E, found F)`
3. wrong hash: `BROKEN at line N: hash mismatch`

If all lines pass print `chain ok: K entries head=H` where `K` is the number of lines and `H` is the last hash (sixteen zeros when there are none).

The hidden tests use different logs: intact chains of different lengths, an edited actor or action, a changed hash digit, deleted or swapped entries, duplicate sequence numbers, uppercase and short hashes, leading zeros, blank lines and smuggled separators. Keep the program reading stdin and writing to stdout.
