---
title: Raw Pointers and Slices
summary: Work with pointer plus length pairs the way C does: null handling, pointer arithmetic, in-place edits and overlapping copies.
minutes: 30
difficulty: hard
tags: ffi, raw-pointers, unsafe
id: rust-ffi-boundaries/raw-pointers
mode: harness
---
C passes arrays as `(ptr, len)`: the address of the first element and a count. In Rust that is `*const T` or `*mut T` plus a `usize`. The pointer carries no length, no lifetime and no promise of validity. It can be null, dangling or misaligned, and dereferencing it is `unsafe`: you promise the memory is valid for `len` elements.

## The mechanism

```rust
unsafe fn first(p: *const i32, len: usize) -> Option<i32> {
    if p.is_null() || len == 0 { None } else { Some(*p) }
}
let third = *p.add(2);                       // arithmetic counts elements, not bytes
let s: &[i32] = std::slice::from_raw_parts(p, len); // view the memory as a safe slice
```

`add(n)` moves by `n * size_of::<T>()` bytes. `from_raw_parts` is the bridge back into safe Rust: it needs a non-null, aligned pointer even when `len == 0`, and the memory must not be modified through another pointer while the slice lives. A slice built from a null pointer is undefined behaviour, so handle the null (and usually the empty) case first.

```text
p -> [ 7 ][ 14 ][ 21 ][ 28 ][ 35 ]      len = 5
      p+0   p+1   p+2   p+3   p+4       p+5 is one-past-the-end: valid to form, never to read
```

## Overlap

`memcpy` forbids overlapping ranges; `memmove` allows them. `std::ptr::copy` is memmove and `copy_nonoverlapping` is memcpy, and the second is undefined behaviour on overlap even if it "seems to work". A hand-written loop must pick its direction. If `dst` is after `src` and the ranges overlap, copy backwards; otherwise the forward loop reads bytes it already overwrote.

```text
src = [1 2 3 4 5 .]    copy 5 elements to src+1
forward:  1 1 1 1 1 1  (each write clobbers the next read)
backward: 1 1 2 3 4 5  (correct)
```

## Pitfalls

- Trusting `len` blindly. The callee cannot check it, so the contract must say the pointer is valid for `len` elements.
- Treating a null pointer with a non-zero length as a normal slice. Decide, and document, what it means.
- Mixing up bytes and elements when doing arithmetic on `*mut u8` versus `*mut i32`.
- Holding a `&mut [T]` from `from_raw_parts_mut` while also reading through the original pointer: that is aliasing, which is undefined behaviour.

## The SAFETY convention

Write a `// SAFETY:` comment on each `unsafe` block ("p is non-null and valid for len elements, checked above and promised by the caller") and a `# Safety` section on each `unsafe fn`. If you cannot write the sentence, the block is not yet justified.

## This lesson's exercise

Four C-style helpers: sum, fill, reverse and an overlap-safe copy. In this lesson a null pointer means "empty": it is never an error. The tests work on one buffer through offset pointers to create overlap in both directions.
