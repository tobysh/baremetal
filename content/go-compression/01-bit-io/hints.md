Keep `buf []byte` and `nbits int` in the writer. The next free bit is bit `7 - nbits%8` of the last byte; start a new byte when `nbits%8 == 0`.
===
Write bit by bit: for `i := n-1; i >= 0; i--` take `(v>>uint(i))&1`. It is slow but easy to get right.
===
The reader needs only the data and a bit position. Bit `p` is `data[p/8] >> (7 - p%8) & 1`. Check `n > Remaining()` before moving anything.
