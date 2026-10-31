- `unsafe write_decimal(buf: *mut u8, cap: usize, n: i64) -> isize`: format `n` in decimal ASCII (like `n.to_string()`). Return the full length needed, excluding the NUL. If `cap > 0`, `buf` must be non-null (otherwise return -1 and write nothing); write the first `min(len, cap - 1)` bytes followed by a NUL byte, so the output is always terminated. If `cap == 0`, write nothing; `buf` may be null (a size query). Bytes of `buf` beyond what you wrote are never touched.
- `unsafe dup_bytes(src: *const u8, len: usize) -> *mut u8`: a heap copy of `len` bytes owned by the caller. `len == 0` or a null `src` returns null.
- `unsafe free_bytes(p: *mut u8, len: usize)`: release a block from `dup_bytes`. Null is a no-op.

`i64::MIN` and `0` are among the values tried.
