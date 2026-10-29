---
title: Box<dyn Error>, Sources and Downcasting
summary: Return Box<dyn Error> from application code, wrap an error while keeping its cause, walk the source chain and recover the concrete type with downcast_ref.
minutes: 30
difficulty: medium
tags: errors, trait-objects, source
id: rust-intermediate/boxed-errors-and-sources
mode: harness
---
Library code defines precise error enums. Application code often just wants "any error, please": `Box<dyn Error>` is a trait object that can hold *any* type implementing `std::error::Error`.

```rust
use std::error::Error;

fn read_pair(s: &str) -> Result<(i32, i32), Box<dyn Error>> {
    let (a, b) = s.split_once(',').ok_or("expected 'a,b'")?;   // &str -> Box<dyn Error>
    Ok((a.trim().parse()?, b.trim().parse()?))                  // ParseIntError -> Box<dyn Error>
}
```

`?` converts any `E: Error` into `Box<dyn Error>` using a built-in `From`, and a string slice or `String` also converts, which makes quick errors easy. The cost is that callers cannot `match` on a variant any more.

## Wrapping and source()

When a low-level error is wrapped in a higher-level one, keep the original as the **source** so nothing is lost:

```rust
#[derive(Debug)]
struct LoadError { name: String, source: ParseIntError }

impl Error for LoadError {
    fn source(&self) -> Option<&(dyn Error + 'static)> { Some(&self.source) }
}
```

The `Display` of `LoadError` says what the program was doing (`could not load 'cfg'`); the source says why (`invalid digit found in string`). Printing the whole chain means following `source()` repeatedly until it returns `None`.

## Getting the type back

A `&dyn Error` can be asked whether it is secretly a particular type:

```rust
if let Some(e) = err.downcast_ref::<ParseIntError>() { /* it was a parse error */ }
```

That is how a caller handles one specific failure and passes the others along.

## This lesson's exercise

Write `read_pair`, make `LoadError` report its source, then write `load`, `load_all`, `chain` and `is_int_error`.
