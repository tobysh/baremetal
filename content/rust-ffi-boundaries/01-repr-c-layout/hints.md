Order fields by alignment, largest first: `b: u32`, `d: u16`, then the two `u8`s. That is 4 + 2 + 1 + 1 = 8 with no padding.
===
`Report { size: size_of::<Wide>(), align: align_of::<Wide>(), off_b: offset_of!(Wide, b), ... }`. Write a helper per type or just fill the struct twice.
===
`to_bytes`: start with `let mut out = [0u8; 12];` (padding is already zero), then `out[offset_of!(Wide, a)] = w.a;` and for wider fields `let o = offset_of!(Wide, b); out[o..o + 4].copy_from_slice(&w.b.to_le_bytes());`.
