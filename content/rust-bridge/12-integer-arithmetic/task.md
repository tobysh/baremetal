Complete these in the starter:

- `fn checked_factorial(n: u32) -> Option<u64>`: `n!`, or `None` when it does not fit in a `u64` (so `20!` works and `21!` is `None`). `0! = 1`.
- `fn fnv1a(data: &[u8]) -> u32`: the 32-bit FNV-1a hash. Start with `hash = 2166136261`; for each byte: `hash ^= byte`, then `hash = hash.wrapping_mul(16777619)`.
- `fn clamp_to_i8(x: i32) -> i8`: the value limited to `-128..=127`, **not** wrapped. Use `try_from` or `clamp`.
- `fn total_saturating(v: &[u8]) -> u8`: the sum of the bytes, stopping at 255 instead of wrapping.
- `fn average(a: i32, b: i32) -> i32`: the mean of two numbers, rounded toward negative infinity, without overflowing even for `i32::MAX` and `i32::MAX`. Widen to `i64` for the sum.
- `fn bits_set(x: u32) -> u32`: how many bits are 1, **without** `count_ones` (loop with shifts or `x &= x - 1`; careful that `x - 1` for `x = 0` would underflow, so stop when `x == 0`).
- `fn set_bit(x: u32, i: u32, on: bool) -> u32`: `x` with bit `i` (0 is the least significant) set or cleared. If `i >= 32`, return `x` unchanged.
