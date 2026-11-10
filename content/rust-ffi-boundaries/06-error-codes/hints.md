Every function follows the same rhythm: check pointers, check the input, do the work, and write `*out` only at the very end on success.
===
`checked_div`: `i32::checked_div` returns `None` for both a zero divisor and `MIN / -1`, so it cannot tell you which code to return on its own. Test the divisor yourself, then let `None` mean overflow.
===
`parse_u8`: first look at every byte (any non-digit is `BAD_INPUT`, even after a huge prefix), then accumulate in a wider integer and report `OVERFLOW` as soon as the value passes 255, so a 30-digit string never overflows the accumulator. Leading zeros are fine.
===
`guarded`: look at `std::panic::catch_unwind` and what it returns when the closure panics. Remember `out` must stay untouched in that case, and the null check comes before `f` is called at all.
