Start with `is_pow2`: a power of two has exactly one bit set, and `a & (a - 1)` clears the lowest set bit.
===
`align_up` has two failure cases to test before doing any arithmetic: bad `a`, and `n > SIZE_MAX - (a - 1)`.
===
`align_down` is `n & ~(a - 1)`. `padding_for` is `align_up(addr, a) - addr` but must not overflow for addresses near the top; compute it as `(a - (addr & (a - 1))) & (a - 1)`.
===
`layout`: keep a running `off`. For each member, `off = align_up(off, align[i])`, store it, then `off += size[i]`. Track the largest alignment and round the final `off` up to it.
