---
title: Error Codes and Panic Containment
summary: Report failure across a boundary with integer codes and out-parameters, never overwrite outputs on error, and stop panics from unwinding into the caller.
minutes: 30
difficulty: hard
tags: ffi, errors, panics
id: rust-ffi-boundaries/error-codes
mode: harness
---
C has no `Result`. A boundary function returns an `int` status (0 means success, negative means an error) and delivers its real result through an **out-parameter**: `int checked_div(int a, int b, int *out)`.

Conventions worth keeping:

- Validate pointers first and return a distinct code for null.
- On error, leave `*out` untouched, so the caller never reads a half-written value.
- Keep one table of codes and one function mapping code to message, as `strerror` does.
- Never let a Rust panic unwind into foreign code. Wrap boundary work in `std::panic::catch_unwind` and convert a panic into a code.

```rust
let r = std::panic::catch_unwind(|| f(arg)); // Err(_) if f panicked
```

The default panic hook still prints a message to stderr; that is harmless here.

## This lesson's exercise

Three boundary functions, a message table and a conversion to `Result`. The tests check codes for every failure kind and that outputs are untouched on error.
