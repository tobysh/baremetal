---
title: Owned Handles with Box::into_raw
summary: Hand a heap value to foreign code as an opaque pointer, use it through the pointer, and take it back with Box::from_raw to free it exactly once.
minutes: 30
difficulty: hard
tags: ffi, ownership, box
id: rust-ffi-boundaries/owned-handles
mode: harness
---
C libraries return opaque handles: `Counter *counter_new(void)`, then `counter_bump(c)`, then `counter_free(c)`. The caller never looks inside. In Rust:

- `Box::into_raw(Box::new(x))` gives up ownership and returns a `*mut T`. Rust will **not** drop it.
- `&mut *p` borrows it back while the handle is live.
- `Box::from_raw(p)` takes ownership again; dropping that box frees the value.

```rust
let p: *mut Counter = Box::into_raw(Box::new(Counter::new(2)));
unsafe { (*p).total += 2; drop(Box::from_raw(p)); }   // exactly one from_raw per into_raw
```

Forget `from_raw` and the value leaks. Call it twice, or use the pointer afterwards, and it is a double free or use after free. The type system no longer helps, so the API must document it: `_free(NULL)` is a harmless no-op, as in C's `free`.

## Checking ownership with Drop

The starter gives `Counter` a `Drop` impl that decrements a thread-local live count, and `Counter::new` increments it. The tests assert the count returns to zero: a leak shows up as a number above zero. This catches leaks but not use after free; real tools for that are Miri and Valgrind, which this runner does not offer.

## This lesson's exercise

Write the four functions of a handle API.
