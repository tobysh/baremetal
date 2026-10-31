`write_decimal`: `let s = n.to_string(); let need = s.len();` then handle `cap == 0`, the null check, `let k = need.min(cap - 1); std::ptr::copy_nonoverlapping(s.as_ptr(), buf, k); *buf.add(k) = 0; need as isize`.
===
`dup_bytes`: `let b: Box<[u8]> = std::slice::from_raw_parts(src, len).to_vec().into_boxed_slice(); Box::into_raw(b) as *mut u8`.
===
`free_bytes`: `drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(p, len)))`, after the null check.
