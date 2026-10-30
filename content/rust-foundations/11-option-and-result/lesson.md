---
title: Option and Result basics
summary: Represent "maybe nothing" with Option and "maybe failed" with Result, and handle both with match, map, unwrap_or and friends.
minutes: 15
difficulty: easy
tags: option, result, errors
id: rust-foundations/option-and-result
mode: harness
---
C signals "nothing here" or "it failed" with magic values: `NULL`, `-1`, `EOF`, `errno`. Nothing forces you to check. Rust has two ordinary enums for these jobs, and the compiler will not let you use the inside of one without dealing with the other case.

## Option: a value or nothing

```rust
enum Option<T> {
    Some(T),
    None,
}
```

```rust
fn find_index(v: &[i32], target: i32) -> Option<usize> {
    for (i, &x) in v.iter().enumerate() {
        if x == target {
            return Some(i);
        }
    }
    None
}
```

Use `match` to handle both cases:

```rust
match find_index(&[5, 6, 7], 6) {
    Some(i) => println!("found at {}", i),
    None => println!("not there"),
}
```

## Result: success or an error

```rust
enum Result<T, E> {
    Ok(T),
    Err(E),
}
```

`Result` is `Option` with a reason attached to the failure case.

```rust
fn parse_digit(c: char) -> Result<u32, String> {
    match c.to_digit(10) {
        Some(d) => Ok(d),
        None => Err(format!("{} is not a digit", c)),
    }
}
```

Note that `to_digit` returned an `Option` and we turned it into a `Result` by supplying the message.

## Shortcuts

Writing `match` every time is wordy. The common patterns have methods:

| method | meaning |
|---|---|
| `opt.unwrap_or(d)` | the value, or `d` if `None` (same on `Result`) |
| `opt.map(f)` | apply `f` to the inside if there is one, keep `None` otherwise |
| `opt.is_some()`, `is_none()` | quick checks (`is_ok()`, `is_err()` on `Result`) |
| `opt.ok_or(e)` | turn `Option` into `Result`, using `e` as the error |
| `if let Some(x) = opt { ... }` | run code only for `Some` |

```rust
let n: Option<i32> = Some(4);
let doubled = n.map(|x| x * 2);       // Some(8)
let value = doubled.unwrap_or(0);     // 8
```

## Avoid unwrap in real code

`opt.unwrap()` returns the inside or **panics** (crashes the thread) if it is `None`. It is fine in quick experiments and tests, but in a program, handle the other case. The next lesson covers the `?` operator, which passes errors up to the caller neatly.

## Parsing is a Result

Text-to-number conversion can fail, so it returns a `Result`:

```rust
let a = "42".parse::<i32>();    // Ok(42)
let b = "x".parse::<i32>();     // Err(ParseIntError)
```

`map_err` changes the error type, which is handy for turning the standard error into a `String`:

```rust
"x".parse::<i32>().map_err(|_| "not a number".to_string())
```

## This lesson's exercise

Six small functions, each returning or consuming an `Option` or `Result`. Use `match` or the shortcuts, your choice.
