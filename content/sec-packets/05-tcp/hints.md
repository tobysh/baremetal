The TCP offset is `(p[12] >> 4) * 4`, not the whole byte.
===
Check `header >= 20 && header <= n` before subtracting.
