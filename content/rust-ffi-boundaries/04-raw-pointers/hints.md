Start every function with `if p.is_null() { return ...; }`. Then `std::slice::from_raw_parts(p, len)` (or `_mut`) gives you safe slice methods: `iter().map(|&x| x as i64).sum()`, `fill`, `reverse`.
===
`len == 0` with a non-null pointer is fine to turn into a slice, but you can also return early.
===
`copy_raw` is one call: `std::ptr::copy(src, dst, len)`. Convince yourself why `copy_nonoverlapping` would be wrong here.
