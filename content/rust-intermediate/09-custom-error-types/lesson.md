---
title: Custom Error Types
summary: Model what can go wrong as an enum, implement Display and std::error::Error for it, and use ok_or and map_err to turn Options and foreign errors into your own.
minutes: 30
difficulty: medium
tags: errors, enums, display
id: rust-intermediate/custom-error-types
mode: harness
---
`Result<T, String>` works, but a string tells the caller nothing it can act on. A real error type is an **enum**: one variant per way the operation can fail, carrying the data that explains it.

```rust
#[derive(Debug, PartialEq)]
enum ConfigError {
    NoSettings,
    MissingEquals(usize),                       // line number
    BadNumber { line: usize, text: String },
    Duplicate(String),
}
```

A caller can now `match` on the cause, retry or report precisely, and tests can compare errors with `assert_eq!`.

## Display and Error

Two traits make an enum a proper error:

```rust
impl fmt::Display for ConfigError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            ConfigError::NoSettings => write!(f, "no settings found"),
            ConfigError::MissingEquals(n) => write!(f, "line {n}: expected key = value"),
            // ...
        }
    }
}
impl std::error::Error for ConfigError {}
```

`Display` is the human message. `Error` requires `Debug + Display` and nothing else; the empty `impl` is enough. With it, your error can be boxed into `Box<dyn Error>`, printed by `main`, and wrapped by other errors.

## Converting into your error

- `Option::ok_or(err)` turns `None` into `Err(err)`: `line.split_once('=').ok_or(ConfigError::MissingEquals(n))?`.
- `Result::map_err(|e| ...)` replaces the error when the original (such as `ParseIntError`) is not the one you want to expose: `val.parse().map_err(|_| ConfigError::BadNumber { ... })?`.

When the conversion is mechanical, implement `From` instead; the next lesson does exactly that.

## This lesson's exercise

The starter has the enum and an empty `impl Error`. Write `Display` and `parse_config`.
