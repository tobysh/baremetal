Encoder loop: `for v >= 0x80 { dst = append(dst, byte(v)|0x80); v >>= 7 }` then append the last byte.
===
Decoder: for byte `i`, OR `uint64(c&0x7f) << (7*i)` into the value; stop when `c < 0x80`. If you finish the loop without stopping, the input ended: `ErrShort`.
===
Overflow: at `i == 9` (the 10th byte), `c > 1` means bits would be lost, including when `c` has its continuation bit set.
===
Non-canonical: when you stop on `c == 0` and `i > 0`.
===
Zigzag: `uint64(v<<1) ^ uint64(v>>63)`. Back: `int64(u>>1) ^ -int64(u&1)`.
