`write_decimal` follows snprintf: compute the whole text first, because the return value is its length no matter how much fits. Only then decide how much to copy.
===
Handle `cap == 0` (write nothing, null allowed) before the null-buffer check. Otherwise copy `min(len, cap - 1)` bytes and then write the NUL right after them, never at index `cap`.
===
`dup_bytes` and `free_bytes` must agree on one allocation shape. A `Vec` may have spare capacity, so convert to a boxed slice (`into_boxed_slice`), then to a raw pointer, and rebuild a box of that exact length when freeing.
===
If freeing misbehaves, remember a raw `*mut u8` forgets its length: `slice_from_raw_parts_mut(p, len)` is how you rebuild a `*mut [u8]` for `Box::from_raw`. Empty input and null never allocate.
