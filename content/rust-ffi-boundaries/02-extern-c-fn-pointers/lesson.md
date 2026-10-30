---
title: extern "C" Functions and Function Pointers
summary: Define functions with the C calling convention, store them in a table as function pointers and call them through the pointer.
minutes: 20
difficulty: medium
tags: ffi, extern-c, function-pointers
id: rust-ffi-boundaries/extern-c-fn-pointers
mode: harness
---
A C library hands you a function as an address: `int (*op)(int, int)`. In Rust the matching type is `extern "C" fn(i32, i32) -> i32`. The `extern "C"` part fixes the calling convention (which registers carry arguments); a plain Rust `fn` has no guaranteed convention and cannot be passed across a boundary.

```rust
type BinOp = extern "C" fn(i32, i32) -> i32;

extern "C" fn add(a: i32, b: i32) -> i32 { a.wrapping_add(b) }

let op: BinOp = add;        // a function item coerces to a pointer
let r = op(2, 3);           // an ordinary call through the pointer
```

Function pointers are `Copy`, compare by address, and (unlike closures) carry no captured data. That is why C callbacks need a separate context pointer, which the next lesson covers.

## No unwinding across the line

A panic must not unwind out of an `extern "C"` function. Rust aborts the process if it tries. So boundary functions should not panic: this lesson uses `wrapping_*` arithmetic so `i32::MAX + 1` is defined behaviour, not an overflow panic.

## This lesson's exercise

Write three `extern "C"` operations, a name lookup returning `Option<BinOp>` (so an unknown name is `None`, like a null pointer in C), and a fold that applies an operation supplied by the caller. The tests call your fold with their own `extern "C"` function too.
