Check `n >= 8` first; only then is it safe to read the length from `p + 4`. The length is two big-endian bytes: `(p[4] << 8) | p[5]`.
===
There are two bounds on the declared length. Below 8 it cannot even cover the header, and above `n` it claims bytes that were not captured. Test `len < 8 || len > n` and return -1.
===
After those checks, the payload size is `len - 8`, not `n - 8`. A packet with trailing padding is the case that tells the two apart.
===
Store the result through `*out` last. Using a `size_t` for `len` avoids a sign mix-up when comparing it with `n`, and the 8-byte check keeps `len - 8` from wrapping.
