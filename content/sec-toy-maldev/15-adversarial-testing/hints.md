SeedFor: fold bytes one at a time with h ^= b; h *= prime. Do the 8 index bytes with the same loop.
===
In Mutants, write small helpers: rebuild(header []byte, code []byte) that recomputes length and CRC, and a copy-on-write mutate so valid is never touched.
===
Minimize: try removing windows of size len/2, len/4, ... down to 1. After a successful removal stay at the same position. At size 1, repeat whole passes until a pass removes nothing.
