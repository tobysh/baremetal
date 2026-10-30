---
title: Callbacks with a Context Pointer
summary: Pass a function pointer plus an opaque context pointer, cast it back inside the callback, and use the return value to stop early.
minutes: 30
difficulty: hard
tags: ffi, callbacks, raw-pointers
id: rust-ffi-boundaries/callbacks-and-context
mode: harness
---
C has no closures, so a function that wants to call you back takes two arguments: a function pointer and a `void *ctx` it passes straight back. You decide what `ctx` points to.

```rust
use std::ffi::c_void;
type Visit = extern "C" fn(ctx: *mut c_void, value: i32) -> i32;

struct Sum(i64);
extern "C" fn add_up(ctx: *mut c_void, v: i32) -> i32 {
    let sum = unsafe { &mut *(ctx as *mut Sum) };   // you promise ctx is a live Sum
    sum.0 += v as i64;
    0
}

let mut total = Sum(0);
unsafe { for_each(xs.as_ptr(), xs.len(), add_up, &mut total as *mut Sum as *mut c_void) };
```

## The contract, written down

Nothing checks the cast. The caller and the callback must agree on the type behind `ctx`, and that memory must outlive the call. A convention such as "a non-zero return means stop" is also only a contract. State both in doc comments, as C headers do.

## This lesson's exercise

Write the iteration function, a statistics callback and a search callback that stops early. The tests call `for_each` with their own callbacks, including ones that record the visiting order and stop at a chosen element.
