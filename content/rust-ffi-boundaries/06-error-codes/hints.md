`checked_div`: `a.checked_div(b)` returns `None` for both zero divisor and overflow, so test `b == 0` first, then use `None` as `OVERFLOW`.
===
`parse_u8`: first check every byte with `is_ascii_digit()` (any non-digit is `BAD_INPUT`, even after a large prefix), then accumulate in a `u32` and return `OVERFLOW` as soon as it exceeds 255 so it never grows large.
===
`guarded`: `match std::panic::catch_unwind(|| f(arg)) { Ok(v) => { *out = v; OK } Err(_) => PANICKED }`.
