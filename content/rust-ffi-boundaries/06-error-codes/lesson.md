---
title: Error Codes and Panic Containment
summary: Report failure across a boundary with integer codes and out-parameters, never overwrite outputs on error, and stop panics from unwinding into the caller.
minutes: 30
difficulty: hard
tags: ffi, errors, panics
id: rust-ffi-boundaries/error-codes
mode: harness
---
C has no `Result` and no exceptions. A boundary function returns an `int` status (0 means success, negative means an error) and delivers its real result through an **out-parameter**: `int checked_div(int a, int b, int *out)`. The caller checks the code before reading `out`, so the library must never leave `out` half written.

## Conventions worth keeping

- Validate pointers first and return a distinct code for null, so a mistake is reported, not crashed on.
- On error, leave `*out` untouched. A caller that ignores the code then reads its own old value, not a corrupted one.
- Keep one table of codes and one function mapping code to message, as `strerror` does. Codes are part of the ABI: never renumber them.
- Check cheap, unambiguous conditions in a fixed order and document it, because tests (and callers) depend on which code wins when several apply.

```text
checked_div(a, b, out)
   out null?        -> NULL_ARG    (nothing written)
   b == 0?          -> BAD_INPUT   (nothing written)
   MIN / -1?        -> OVERFLOW    (nothing written)
   else *out = a/b  -> OK
```

## Panics must not cross the boundary

Unwinding a Rust panic through a C frame is not supported: in an `extern "C"` function it aborts the process, and across a plain function it is undefined behaviour in the C caller. Wrap boundary work in `std::panic::catch_unwind` and convert a panic into a code.

```rust
let r = std::panic::catch_unwind(|| f(arg)); // Err(_) if f panicked
```

`catch_unwind` only catches unwinding panics (not `panic = "abort"`), and the closure must be `UnwindSafe`; `AssertUnwindSafe` is the escape hatch when you can justify it. The default panic hook still prints a message to stderr; that is harmless here.

## Pitfalls

- Letting arithmetic panic or wrap silently. Use `checked_*` and map `None` to a code. A very long digit string must report overflow, not loop forever or panic in debug builds.
- Reading `s` as UTF-8. A C string is bytes: check ASCII digits yourself.
- Writing `*out` before you know the call succeeds.

## The SAFETY convention

Each `unsafe` block carries a `// SAFETY:` comment (for instance "out was checked non-null above and the caller promises it is writable"), and each `unsafe fn` documents its pointer requirements under `# Safety`. The error codes themselves are the safe half of the contract: everything the caller can get wrong should map to one.

## This lesson's exercise

Three boundary functions, a message table and a conversion to `Result`. The tests check the code for every failure kind, that outputs are untouched on error, and that panics become `PANICKED`.
