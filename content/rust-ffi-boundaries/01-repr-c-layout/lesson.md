---
title: repr(C) Layout and Padding
summary: Control field order and padding so a struct has a known layout, measure it with size_of and offset_of, and serialize it byte by byte.
minutes: 25
difficulty: medium
tags: ffi, repr-c, layout
id: rust-ffi-boundaries/repr-c-layout
mode: harness
---
Plain Rust structs have an unspecified layout: the compiler may reorder fields to save space. A C compiler never reorders: fields sit in declaration order, each aligned to its own size, and the whole struct is padded to a multiple of its largest alignment. Code on the other side of a boundary relies on that, so a boundary type is marked `#[repr(C)]`.

## Measuring a layout

```rust
use std::mem::{align_of, offset_of, size_of};

#[repr(C)]
struct S { a: u8, b: u32 }

size_of::<S>();        // 8: a at 0, three padding bytes, b at 4
align_of::<S>();       // 4
offset_of!(S, b);      // 4
```

## Padding is wasted space

`Wide { a: u8, b: u32, c: u8, d: u16 }` puts `a` at 0, pads to 4 for `b`, puts `c` at 8, pads one byte for `d` at 10 and ends at 12. The same four fields ordered largest first fit in 8 bytes. Because `repr(C)` keeps your order, **you** must pick a good one.

## Padding bytes are not data

When you write a struct into a byte buffer, padding must be zero (never uninitialised memory leaking out), and multi-byte integers need an explicit byte order. `u32::to_le_bytes` gives that.

## This lesson's exercise

Fill in `Tight` (the same four fields, `repr(C)`, reordered to be 8 bytes), the two layout reports, `widen` and `to_bytes`. The tests build values from many inputs and compare against bytes written out by hand. They assume a little-endian target, which is what the runner uses.
