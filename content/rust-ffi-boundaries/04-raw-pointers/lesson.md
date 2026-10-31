---
title: Raw Pointers and Slices
summary: Work with pointer plus length pairs the way C does: null handling, pointer arithmetic, in-place edits and overlapping copies.
minutes: 30
difficulty: hard
tags: ffi, raw-pointers, unsafe
id: rust-ffi-boundaries/raw-pointers
mode: harness
---
C passes arrays as `(ptr, len)`. In Rust that is `*const T` or `*mut T` plus a `usize`. Raw pointers can be null, dangling or unaligned, and dereferencing one is `unsafe`: you promise it is valid for `len` elements.

```rust
unsafe fn first(p: *const i32, len: usize) -> Option<i32> {
    if p.is_null() || len == 0 { None } else { Some(*p) }
}
let third = *p.add(2);                       // pointer arithmetic counts elements, not bytes
let s: &[i32] = std::slice::from_raw_parts(p, len); // view the memory as a safe slice
```

Rules of thumb: check null first, never read past `len`, and remember `from_raw_parts` still requires a non-null, aligned pointer even for `len == 0`, so handle the empty case before you build a slice.

## Overlap

`memcpy` forbids overlapping ranges; `memmove` allows them. `std::ptr::copy` is memmove and `copy_nonoverlapping` is memcpy. A hand-written loop must pick its direction: copying forward over a range that overlaps ahead of itself reads bytes you already overwrote.

## This lesson's exercise

Four C-style helpers. In this lesson a null pointer means "empty": it is never an error. The tests work on the same buffer through offset pointers to create overlap.
