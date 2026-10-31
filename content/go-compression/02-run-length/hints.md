Compute the run length `r` starting at `i` first: `for i+r < len(data) && data[i+r] == data[i] && r < 129 { r++ }`.
===
If `r >= 3`, flush pending literals, append `byte(128 + r - 2)` and the value, advance `i` by `r`. Otherwise append only `data[i]` to the literals and advance by 1: the next position will recompute its own run.
===
A literal buffer that reaches 128 bytes is flushed right away. Flushing writes `byte(len(lit)-1)` then the bytes. Do not forget the final flush.
===
Decoder: read control byte; for literals check `i+n <= len(enc)` before slicing; for repeats check there is a byte to repeat.
