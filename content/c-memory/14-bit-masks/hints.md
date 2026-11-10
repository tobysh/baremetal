Keep the register in a `uint32_t` (from `<stdint.h>`) that starts at 0. Read each command word with `scanf("%15s", cmd)` and compare it with `strcmp`; every command except `show` is followed by a bit number.
===
Make a mask with `1u << n`. Then `set` is `x | mask`, `clear` is `x & ~mask`, and `toggle` is `x ^ mask`. Write them as small functions that take `x` and `n` and return the new value.
===
For `test`, check `(x & mask) != 0`. That already gives exactly 1 or 0.
===
Print with `printf("flags: 0x%08X (%u)\n", x, x)`. Cast to `unsigned` if your compiler warns. Try bit 31 first: `1u << 31` must be unsigned.
