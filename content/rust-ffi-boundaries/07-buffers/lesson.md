---
title: Buffers: snprintf-Style Output and Owned Byte Blocks
summary: Fill a caller-supplied buffer without overflowing it, report the size you needed, and hand out and reclaim a heap block given only a pointer and length.
minutes: 30
difficulty: hard
tags: ffi, buffers, ownership
id: rust-ffi-boundaries/buffers
mode: harness
---
Who owns the memory? Boundary APIs answer in one of two ways, and mixing them up is the classic source of heap corruption.

## Caller allocates (`snprintf`)

You pass `buf` and `cap`. The function writes at most `cap` bytes including a terminating NUL, and returns the length the full result *needs* (excluding the NUL). If the return value is `>= cap` the output was truncated. Calling with `buf = NULL, cap = 0` is the standard way to ask for the size first, then you allocate exactly that plus one and call again.

```text
write_decimal(buf, cap = 4, n = -12345)  needs 6
buf:  '-' '1' '2' 0 | untouched ...       returns 6, wrote 3 + NUL
          ^ min(len, cap-1) bytes, then NUL; never past cap
```

## Callee allocates

The library returns a pointer and the caller must give it back to the matching free function. A raw pointer to bytes does not remember its length, so the free function needs it too: build the allocation from a `Box<[u8]>`, and rebuild the same box with `std::ptr::slice_from_raw_parts_mut(p, len)` and `Box::from_raw`. Freeing with a different length, or with C's `free()`, is undefined behaviour because Rust's allocator is not C's.

```rust
let b: Box<[u8]> = bytes.to_vec().into_boxed_slice();   // capacity == len
let p = Box::into_raw(b) as *mut u8;
// SAFETY: p and len came from the box above; freed exactly once.
let back = unsafe { Box::from_raw(std::ptr::slice_from_raw_parts_mut(p, len)) };
```

`into_boxed_slice` matters: a `Vec` may have spare capacity, and rebuilding with only `len` would free the wrong size.

## Pitfalls

- Writing `cap` bytes plus the NUL: the terminator counts inside `cap`.
- Returning the written length instead of the needed length, which makes truncation invisible.
- Zero-length allocations: a zero-size box has a dangling, non-null pointer that must never be passed to `free`. The convention here is to return null for empty input.
- Null buffer with `cap > 0`: report an error instead of writing to address zero.
- Touching bytes beyond what you wrote; callers may keep data there.

## The SAFETY convention

Each `unsafe` block has a `// SAFETY:` line ("buf is valid for cap bytes, cap > 0 and buf non-null checked above") and each `unsafe fn` lists its requirements under `# Safety`, including who frees what and with which length.

## Honest limit

A leak or a wrong-length free cannot be detected by these tests. They check contents, truncation and null handling; use Miri or a sanitizer on real code.

## This lesson's exercise

`write_decimal` (caller allocates) and `dup_bytes` / `free_bytes` (callee allocates). Edge values such as `0` and `i64::MIN` are tried.
