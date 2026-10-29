`checked_factorial`: `let mut acc: u64 = 1; for k in 2..=n as u64 { acc = acc.checked_mul(k)?; } Some(acc)`. The `?` returns `None` as soon as one step overflows. `fnv1a`: a plain `fold` over the bytes: `data.iter().fold(2166136261u32, |h, &b| (h ^ b as u32).wrapping_mul(16777619))`.
===
`clamp_to_i8`: `x.clamp(-128, 127) as i8`, since after clamping the cast is exact. `total_saturating`: `v.iter().fold(0u8, |acc, &b| acc.saturating_add(b))`. `average`: `((a as i64 + b as i64).div_euclid(2)) as i32`; `div_euclid` rounds down, but `>> 1` on the `i64` does as well.
===
`bits_set`: `let mut n = 0; while x != 0 { x &= x - 1; n += 1; }` (each step clears the lowest set bit). `set_bit`: `if i >= 32 { return x; } let m = 1u32 << i; if on { x | m } else { x & !m }`.
