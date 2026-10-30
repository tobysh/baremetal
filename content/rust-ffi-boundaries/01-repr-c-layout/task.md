Starter types: `Wide` is given. Complete:

- `Tight`: keep the field names `a: u8, b: u32, c: u8, d: u16` but reorder them so `size_of::<Tight>()` is 8.
- `wide_report()` and `tight_report()`: return a `Report` with the size, alignment and the offsets of `b`, `c` and `d` of that type. Measure them with `size_of`, `align_of` and `offset_of!`; do not type the numbers in.
- `widen(&Tight) -> Wide`: copy the fields across.
- `to_bytes(&Wide) -> [u8; 12]`: the struct's bytes as C would lay them out, little-endian, with every padding byte set to 0. Place each field using `offset_of!(Wide, field)`.

Other field values are tried, so write the general rule.
