---
title: Result and the ? Operator
summary: Return Result from your own functions, propagate failures with ?, and nest Option inside Result for lookups that can also fail to parse.
minutes: 20
difficulty: easy
tags: errors, result, question-mark
id: rust-intermediate/result-and-question-mark
mode: harness
---
`Result<T, E>` is either `Ok(value)` or `Err(error)`. A function that can fail returns one, and its caller must decide what to do about the failure. No exceptions, no hidden control flow.

## The ? operator

Writing `match` after every call buries the real logic. `?` does the common thing for you:

```rust
fn parse_sum(a: &str, b: &str) -> Result<i32, ParseIntError> {
    let x: i32 = a.trim().parse()?;   // on Err: return that Err immediately
    let y: i32 = b.trim().parse()?;
    Ok(x + y)
}
```

`expr?` unwraps `Ok(v)` to `v`, or **returns early** with the `Err`. It only works inside a function whose return type is also a `Result` (or `Option`, for `Option`s) with a compatible error.

## Result around Option

Sometimes a lookup can fail in two different ways: the input may be malformed (an error) or the thing you want may simply not be there (a normal outcome). `Result<Option<T>, E>` expresses that: `Err` means malformed, `Ok(None)` means "valid input, no answer".

## Handling errors instead of passing them on

- `unwrap_or(default)` supplies a value for the `Err` case.
- `match` or `if let Err(e)` handles it. `e.to_string()` and `{e}` print the error's message, because standard errors implement `Display`.
- `map_err(|e| ...)` converts the error on the way through.

## Parse errors

`str::parse::<i32>()` returns `Result<i32, ParseIntError>`. The message for `"abc"` is `invalid digit found in string`, for `""` it is `cannot parse integer from empty string`.

## This lesson's exercise

Write five small functions: `parse_sum`, `sum_all`, `first_even`, `parse_or` and `describe`.
