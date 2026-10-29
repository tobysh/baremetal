The state right after the first output is `((uint32_t)o1 << 16) | low` for some `low` between 0 and 65535. Write a helper `static uint32_t step(uint32_t s) { return s * 1664525u + 1013904223u; }`.
===
For each `low`, compute `s2 = step(s)` and `s3 = step(s2)`. If `(s2 >> 16) == o2` and `(s3 >> 16) == o3`, this candidate state is consistent with everything you saw.
===
When you find it, return `(uint16_t)(step(s3) >> 16)`. If the first match happens to be a false positive on some inputs, keep looking; but with three outputs it should not.
