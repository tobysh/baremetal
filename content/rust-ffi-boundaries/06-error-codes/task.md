Codes (given): `OK 0`, `NULL_ARG -1`, `BAD_INPUT -2`, `OVERFLOW -3`, `PANICKED -99`.

- `unsafe checked_div(a: i32, b: i32, out: *mut i32) -> i32`: null `out` gives `NULL_ARG`; `b == 0` gives `BAD_INPUT`; `i32::MIN / -1` gives `OVERFLOW`; otherwise store `a / b` and return `OK`. On any error `*out` is not written.
- `unsafe parse_u8(s: *const u8, len: usize, out: *mut u8) -> i32`: parse ASCII decimal digits. Null `out`, or null `s` with `len > 0`, gives `NULL_ARG`. An empty string or any non-digit gives `BAD_INPUT`. A value above 255 gives `OVERFLOW` (leading zeros are allowed, so `"0000255"` is fine, and a very long digit string must report `OVERFLOW` without panicking). `*out` is only written on `OK`.
- `unsafe guarded(f: fn(i32) -> i32, arg: i32, out: *mut i32) -> i32`: call `f(arg)`; store the result and return `OK`, or return `PANICKED` if `f` panics. Null `out` gives `NULL_ARG` and `f` is not called.
- `status_message(code: i32) -> &'static str`: `"ok"`, `"null argument"`, `"bad input"`, `"overflow"`, `"panicked"`, and `"unknown"` for anything else.
- `into_result(code: i32) -> Result<(), i32>`: `Ok(())` for `OK`, otherwise `Err(code)`.
