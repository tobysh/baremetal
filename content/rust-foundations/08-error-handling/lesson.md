---
title: Error handling
summary: Handle failure without exceptions or magic return codes, using Result, the ? operator, parse and map_err.
minutes: 18
difficulty: hard
tags: errors
id: rust-foundations/error-handling
mode: harness
---
In C, a function that can fail returns a special value (`-1`, `NULL`) or sets a global `errno`, and nothing forces the caller to look. Many languages use exceptions instead, which jump out of a function invisibly. Rust has **no exceptions**. A function that can fail says so in its return type, and the compiler makes sure the caller deals with it.

## Result

```rust
enum Result<T, E> {
    Ok(T),
    Err(E),
}
```

`Result` is an enum like `Option`, but the failure case carries information. `T` is the success type and `E` the error type.

```rust
fn half(n: i32) -> Result<i32, String> {
    if n % 2 == 0 {
        Ok(n / 2)
    } else {
        Err(format!("{} is odd", n))
    }
}
```

You handle it with `match`, just like any enum:

```rust
match half(7) {
    Ok(v) => println!("got {}", v),
    Err(e) => println!("failed: {}", e),
}
```

## Parsing

Turning text into a number can fail, so `parse` returns a `Result`. Tell it the target type with the "turbofish" `::<>`:

```rust
let a = "42".parse::<i64>();   // Ok(42)
let b = "abc".parse::<i64>();  // Err(ParseIntError)
```

Also note `" 42".parse::<i64>()` fails. Whitespace is not ignored, so call `.trim()` first.

## The ? operator

Checking every call with `match` gets noisy. Adding `?` after an expression means: if it is `Ok(v)`, give me `v`; if it is `Err(e)`, return that error from the *current function* right now.

```rust
fn double_text(s: &str) -> Result<i64, std::num::ParseIntError> {
    let n = s.parse::<i64>()?;
    Ok(n * 2)
}
```

`?` only works in a function that itself returns a `Result` (or `Option`). The error types must match, which brings us to:

## map_err

`parse` fails with `ParseIntError`, but you may want your function to return `String`. `map_err` converts the error part and leaves success alone:

```rust
let n = s.parse::<i64>().map_err(|_| format!("bad number: {}", s))?;
```

## Common mistakes

- Using `.unwrap()` everywhere. It returns the value but **panics** (crashes) on `Err`. That is fine in tests and quick experiments, but real code should handle the error. Tests use `unwrap_err()` to get the error out of a `Result` that is expected to fail.
- Forgetting the final `Ok(...)` around the success value.
- Using `?` in `main` or another function that returns `()`.

A function you can call and be forced to think about failure is a function you won't forget to check.
