Implement in `starter.go` (it already has `RLEEncode`, `RLEDecode`, `Limits` and `ErrTooLarge`):

- `RLEDecodeLimited(enc, lim)` following the three rules above exactly.
- `Pack(data)`: `[0] + data`, or `[1] + RLEEncode(data)` if that is strictly shorter than `data`.
- `Unpack(enc, lim)`: empty input is `ErrTruncated`; mode 0 returns the rest (`ErrTooLarge` if longer than `MaxOutput`); mode 1 uses `RLEDecodeLimited` on the rest; any other mode is `ErrCorrupt`.

Hidden tests: exact boundaries (output equal to the limit passes, one over fails), the ratio floor, 2000 random token streams against a reference implementation of the rules, every string up to length 5 over `{a,b,c}` (never grows by more than 1), 20 KB of noise (stored, +1), and a 2 MB bomb with a 1 MiB limit where **total memory allocated is measured**.