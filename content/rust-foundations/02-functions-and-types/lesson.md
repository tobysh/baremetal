---
title: Functions and types
summary: Learn Rust's integer and float types, expressions versus statements, implicit return and if as an expression.
minutes: 15
difficulty: easy
id: rust-foundations/functions-and-types
mode: harness
---
A Rust function states every parameter type and the return type after `->`:

```rust
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

Notice there is no `return` and no semicolon after `a + b`. That is deliberate, and explained below.

## Number types

C's `int` changes size between platforms. Rust types say their size in the name:

- `i32`, `i64`: signed integers of 32 and 64 bits
- `u32`, `u64`, `usize`: **unsigned** integers (never negative). `usize` is pointer-sized and is what lengths and indexes use.
- `f64`, `f32`: floating-point numbers

Rust never converts between them silently. Adding an `i32` to an `i64` is a compile error; you convert explicitly with `as`:

```rust
let a: i32 = 5;
let b: i64 = a as i64 + 10;
```

The common mistake is writing `2` where an `f64` is needed. `x * 2` fails if `x` is `f64`; write `x * 2.0`.

## Expressions and statements

A **statement** does something and produces no value, like `let x = 5;`. An **expression** produces a value, like `5 + 1`. A block `{ ... }` is an expression too, and its value is its **last expression without a semicolon**:

```rust
fn double(n: i32) -> i32 {
    n * 2      // no semicolon: this is the return value
}
```

Add a semicolon and it becomes a statement that produces nothing, so the function returns `()` and the compiler complains about a type mismatch. If you see "expected `i32`, found `()`", look for a stray semicolon on the last line. You can still use `return x;` for early exits.

## if is an expression

Because `if` produces a value, it replaces C's `?:` operator:

```rust
let label = if n < 0 { "negative" } else { "not negative" };
```

Both branches must produce the same type, and conditions must be real `bool`s (no "nonzero means true" as in C). Use `&&`, `||` and `!` as you did in C, and `%` for remainder.

```rust
fn is_even(n: i32) -> bool {
    n % 2 == 0
}
```

Hidden tests call your functions as before. For `f64` results, tests check "close enough" rather than exactly equal, because floating-point arithmetic rounds.
