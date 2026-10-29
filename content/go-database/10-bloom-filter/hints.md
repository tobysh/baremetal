Store `bits []uint64`, `m uint32` and `k int`. Set bit `p` with `b.bits[p/64] |= 1 << (p % 64)` and test it with `b.bits[p/64]&(1<<(p%64)) != 0`.
===
Put the position calculation in one helper, for instance `positions(key) []uint32` or a function that calls a callback `k` times. `Add` and `MayContain` then differ only in what they do with each position.
===
`m` rounded up to a multiple of 64: `m = (m + 63) / 64 * 64`. For `k`: `k := int(float64(bitsPerKey)*0.69 + 0.5)`, then clamp.
===
`MayContain` returns `false` as soon as one bit is unset, and `true` only after all `k` bits are set.
===
`Marshal`: `out := []byte{byte(k)}`, `binary.BigEndian.AppendUint32(out, m)`, then `AppendUint64` for each word. `UnmarshalBloom` checks `len(data) == 5 + 8*words` where `words = (m+63)/64`; do the sum in `uint64`.
