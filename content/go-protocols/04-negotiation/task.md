The types, error values and `featureSince` table are given. Implement:

- `EncodeHello(h Hello) []byte`: 10 bytes: `MinVer`, `MaxVer`, `Features` (u32 big endian), `Required` (u32 big endian).
- `DecodeHello(p []byte) (Hello, error)`: as described (6-byte legacy form, ignore trailing bytes, `ErrBadHello` when shorter than 6). It does not validate the version range.
- `Negotiate(local, remote Hello) (Agreement, error)`: the rules above. On error return the zero `Agreement`. The result must not depend on which hello is "local" (the tests swap them).
