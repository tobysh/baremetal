---
title: From and the ? Conversion
summary: Let ? convert errors automatically by implementing From for each error you wrap, and collect values and failures separately.
minutes: 30
difficulty: medium
tags: errors, from, question-mark
id: rust-intermediate/from-and-question-mark
mode: harness
---
In the last lesson you used `map_err` to convert one error into another. When the conversion is mechanical, there is a better way. The `?` operator does more than "return early on Err"; it also **converts** the error with `From::from`:

```rust
let n: i64 = text.parse()?;   // ParseIntError -> your error, if From is implemented
```

`expr?` is, roughly, `match expr { Ok(v) => v, Err(e) => return Err(From::from(e)) }`.

## One From impl per source error

```rust
enum AppError {
    BadInt(ParseIntError),
    BadFloat(ParseFloatError),
    Negative(i64),
}

impl From<ParseIntError> for AppError {
    fn from(e: ParseIntError) -> AppError { AppError::BadInt(e) }
}
```

With one such impl for each error type, a single function can use `?` on integer parsing, float parsing and its own checks, and they all flow into `AppError`. The type of the error each `?` sees decides which `From` runs.

The variants **wrap** the original error instead of throwing it away, so no information is lost and you can still print it.

## Splitting successes and failures

Sometimes you want every result, not the first failure. Instead of `collect::<Result<Vec<_>, _>>()`, loop and push into two vectors: values for `Ok`, messages for `Err`. Printing an error with `to_string()` gives its `Display` text.

## This lesson's exercise

The starter has the enum, stub `From` impls and the `Display` stub. Complete them, then write `parse_scaled`, `total_scaled` and `collect_valid`, using `?` for every conversion.
