---
title: Owned Handles with Box::into_raw
summary: Hand a heap value to foreign code as an opaque pointer, use it through the pointer, and take it back with Box::from_raw to free it exactly once.
minutes: 30
difficulty: hard
tags: ffi, ownership, box
id: rust-ffi-boundaries/owned-handles
mode: harness
---
C libraries return opaque handles: `Counter *counter_new(void)`, then `counter_bump(c)`, then `counter_free(c)`. The caller never looks inside the struct, only passes the pointer back. The hard part is ownership, which C leaves entirely to convention: who frees the value, and when?

## The mechanism

- `Box::into_raw(Box::new(x))` gives up ownership and returns a `*mut T`. Rust will **not** drop it.
- `&mut *p` borrows it back for as long as the handle is live.
- `Box::from_raw(p)` takes ownership again; dropping that box runs `Drop` and frees the memory.

```rust
let p: *mut Counter = Box::into_raw(Box::new(Counter::new(2)));
// SAFETY: p came from into_raw just above and has not been freed.
unsafe { (*p).total += 2; drop(Box::from_raw(p)); }   // exactly one from_raw per into_raw
```

```text
Rust side                         C side
Box<Counter> --into_raw-->  *mut Counter   (opaque, caller holds it)
                                |  bump / total: borrow, never free
Box<Counter> <--from_raw---  counter_free(p)   <- the single owner of the free
```

## Pitfalls

Forget `from_raw` and the value leaks. Call it twice, or use the pointer afterwards, and you get a double free or a use after free. The type system no longer helps, so the API must document the rules:

- Exactly one `_free` per `_new`. After it, the handle is dangling; never reuse it.
- `_free(NULL)` is a harmless no-op, as in C's `free`, so cleanup code does not need an `if`.
- Functions that only look at the handle (`bump`, `total`) borrow it; functions that consume it (`into_inner`) end its life, and the caller must not free it again.
- Never `Box::from_raw` a pointer that did not come from `Box::into_raw` of the same type: a different allocator or layout is undefined behaviour.
- Panics must not cross the boundary; keep these functions panic-free (wrapping arithmetic, no indexing that can fail).

## Checking ownership with Drop

The starter gives `Counter` a `Drop` impl that decrements a thread-local live count, and `Counter::new` increments it. The tests assert the count returns to zero: a leak shows up as a number above zero. This catches leaks and some double frees, but not every use after free; real tools for that are Miri and Valgrind, which this runner does not offer.

## The SAFETY convention

Every `unsafe` block gets a `// SAFETY:` comment naming the promise it relies on ("c is non-null and came from counter_new"). Every `unsafe fn` documents its contract in a `# Safety` section. For handles, the contract is nearly always: non-null or documented as no-op, created by this library, not yet freed.

## This lesson's exercise

Write the five functions of a handle API: create, bump, read, free and consume. The tests count live counters, so every handle must be freed exactly once.
