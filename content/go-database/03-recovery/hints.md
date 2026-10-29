Keep a position `pos` into the slice. At each step you need at least 9 bytes for the header (`op`, `keyLen`, `valLen`); if fewer remain, stop.
===
Read `kl := uint64(binary.BigEndian.Uint32(log[pos+1:]))` and `vl` the same way, then compare `uint64(len(log)-pos-9) < kl+vl`. Doing the sums in `uint64` avoids overflow.
===
Only after the whole record is known to fit do you apply it and move `pos` forward. Set `valid = pos` at the end of each complete record, so a stop mid-record leaves `valid` at the previous boundary.
===
`Recover`: `state, valid := Replay(buf.Bytes())`, then `buf.Truncate(valid)`.
