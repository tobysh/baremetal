---
title: Callbacks with a Context Pointer
summary: Pass a function pointer plus an opaque context pointer, cast it back inside the callback, and use the return value to stop early.
minutes: 30
difficulty: hard
tags: ffi, callbacks, raw-pointers
id: rust-ffi-boundaries/callbacks-and-context
mode: harness
---
C has no closures, so a function that wants to call you back takes two arguments: a function pointer and a `void *ctx` that it passes straight back on every call. The library never looks inside `ctx`; you decide what it points to. This is how `qsort_r`, `pthread_create` and every event API in C carry state into a callback.

## The mechanism

```rust
use std::ffi::c_void;
type Visit = extern "C" fn(ctx: *mut c_void, value: i32) -> i32;

struct Sum(i64);
extern "C" fn add_up(ctx: *mut c_void, v: i32) -> i32 {
    // SAFETY: the caller of for_each passed a live, exclusive `*mut Sum` as ctx.
    let sum = unsafe { &mut *(ctx as *mut Sum) };
    sum.0 += v as i64;
    0                                   // 0 = keep going
}

let mut total = Sum(0);
unsafe { for_each(xs.as_ptr(), xs.len(), add_up, &mut total as *mut Sum as *mut c_void) };
```

Three details carry the whole idea. The callback is `extern "C"` so it uses the C calling convention and can be stored as a plain function pointer. The pointer is erased to `*mut c_void` on the way in and cast back on the way out. And the return value is a second channel: by convention, non-zero means "stop".

```text
for_each(xs, len, cb, ctx)          your Sum on the stack
   |  cb(ctx, xs[0]) -----------> ctx -> [ Sum(0) ]
   |  cb(ctx, xs[1]) -----------> same pointer, same Sum
   '- returns count of calls
```

## What can go wrong

Nothing checks the cast. If the caller passes a `*mut Stats` and the callback casts it to `Sum`, you get silent memory corruption, not an error. Equally, `ctx` must stay alive and unaliased for the whole call: do not let the callback stash it somewhere that outlives `for_each`, and do not touch the original variable while the loop runs, because the callback holds a `&mut` derived from it.

Two more rules from the boundary:

- **Panics must not unwind out of an `extern "C"` function.** Since Rust 1.81 that aborts the process; in older code it was undefined behaviour. A real callback wraps risky work in `catch_unwind` (lesson 6).
- **A callback cannot return an error type**, only an integer. Document what each value means.

## The SAFETY convention

Put a `// SAFETY:` comment above every `unsafe` block saying which promise makes it valid ("ctx is a live `Stats`, supplied by `stats_of`"). Put a `# Safety` section in the doc comment of every `unsafe fn` listing what its caller must guarantee. The comment is the contract, written where a reviewer will look.

## This lesson's exercise

Write the iteration function `for_each` (null and length checks, early stop, a count of calls), a statistics callback and a search callback that stops early. The tests call `for_each` with their own callbacks, including ones that record the visiting order and stop at a chosen element, so your loop must be correct on its own and not just with your callbacks.
