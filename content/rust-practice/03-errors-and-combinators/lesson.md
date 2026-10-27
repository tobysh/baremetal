---
title: Errors and Combinators
summary: Chain Option and Result with map, and_then and ok_or, then write your own error enum with Display and From so the ? operator does the rest.
minutes: 25
difficulty: medium
tags: errors, combinators
id: rust-practice/errors-and-combinators
mode: harness
---
Rust has no exceptions. A function that can fail returns a `Result<T, E>`, and one that might find nothing returns an `Option<T>`. This lesson is about handling both without a forest of `match` blocks.

## Combinators

`Option` and `Result` carry methods that transform the value inside and leave the "nothing" or "error" case untouched:

```rust
let n: Option<i32> = Some(4);
n.map(|x| x * 2);                  // Some(8)
n.filter(|x| x % 2 == 1);          // None
n.and_then(|x| x.checked_sub(10)); // for steps that may themselves fail
n.ok_or("missing");                // Ok(4), or Err("missing") if it was None
```

`map` is for a step that cannot fail. `and_then` is for a step that returns another `Option` (or `Result`), so you do not end up with `Some(Some(..))`. `ok_or` turns an `Option` into a `Result` by supplying the error to use for `None`.

## The ? operator

Inside a function that returns `Result`, `value?` means: "if this is `Ok`, unwrap it; if it is `Err`, return that error right now". It works on `Option` too, inside functions that return `Option`. It is the tidy way to write a chain of fallible steps in a straight line.

## Your own error type

Library functions each have their own error type (`ParseIntError`, `io::Error`). To write one function that can fail in several ways, define an enum:

```rust
#[derive(Debug, PartialEq)]
enum PortError {
    Empty,
    NotANumber(std::num::ParseIntError),
}
```

Two small impls make it pleasant to use:

```rust
impl std::fmt::Display for PortError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            PortError::Empty => write!(f, "port is empty"),
            PortError::NotANumber(e) => write!(f, "not a number: {}", e),
        }
    }
}

impl From<std::num::ParseIntError> for PortError {
    fn from(e: std::num::ParseIntError) -> PortError {
        PortError::NotANumber(e)
    }
}
```

`Display` is what `println!("{}", err)` and `.to_string()` use. `From` is the quiet hero: when `?` meets an `Err(e)`, it calls `From::from(e)` to convert it into your function's error type. So `s.parse::<i64>()?` inside a function returning `Result<_, PortError>` just works.

## This lesson's exercise

You will finish a small settings reader: a `PortError` with `Display` and `From`, a parser, a lookup in `key=value` text, and a combinator warm-up.
