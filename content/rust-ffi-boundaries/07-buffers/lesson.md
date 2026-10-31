---
title: Buffers: snprintf-Style Output and Owned Byte Blocks
summary: Fill a caller-supplied buffer without overflowing it, report the size you needed, and hand out and reclaim a heap block given only a pointer and length.
minutes: 30
difficulty: hard
tags: ffi, buffers, ownership
id: rust-ffi-boundaries/buffers
mode: harness
---
Who owns the memory? Boundary APIs answer in one of two ways.

**Caller allocates** (`snprintf`): you pass `buf` and `cap`. The function writes at most `cap` bytes including a terminating NUL, and returns the length the full result *needs* (excluding the NUL). If the return value is `>= cap` the output was truncated, and calling with `buf = NULL, cap = 0` is the standard way to ask for the size first.

**Callee allocates**: the library returns a pointer and the caller must give it back to the matching free function. A raw pointer to bytes does not remember its length, so the free function needs it too: build the allocation from a `Box<[u8]>`, and rebuild the same box with `std::ptr::slice_from_raw_parts_mut(p, len)` and `Box::from_raw`. Freeing with a different length, or with `free()` from C, is undefined behaviour.

```rust
let b: Box<[u8]> = bytes.to_vec().into_boxed_slice();   // capacity == len
let p = Box::into_raw(b) as *mut u8;
let back = Box::from_raw(std::ptr::slice_from_raw_parts_mut(p, len));
```

## Honest limit

A leak or a wrong-length free cannot be detected by these tests. They check contents and null handling; use Miri or a sanitizer on real code.

## This lesson's exercise

`write_decimal` (caller allocates) and `dup_bytes` / `free_bytes` (callee allocates).
