---
title: Option Combinators
summary: Replace nested match and if let with map, and_then, filter, unwrap_or_else, ok_or, zip and the ? operator on Option.
minutes: 20
difficulty: easy
tags: option, combinators
id: rust-bridge/option-combinators
mode: harness
---
In C a missing value is a null pointer or a magic number like `-1`. In Rust it is `Option<T>`: `Some(value)` or `None`. You can `match` on it, but long chains of matches get noisy. `Option` has methods that do the common shapes for you.

| You want to... | Use |
|---|---|
| change the value inside, if any | `opt.map(\|x\| x + 1)` |
| run a step that may itself fail | `opt.and_then(\|x\| next_step(x))` |
| keep it only if a test passes | `opt.filter(\|x\| *x > 0)` |
| supply a default | `opt.unwrap_or(0)` or `unwrap_or_else(\|\| compute())` |
| try the other one if empty | `a.or(b)` or `a.or_else(\|\| ...)` |
| combine two options | `a.zip(b)` gives `Some((x, y))` only if both are `Some` |
| turn into a `Result` | `opt.ok_or("missing")` |
| look inside without moving | `opt.as_ref()`, `opt.as_deref()` (`Option<String>` to `Option<&str>`) |

```rust
fn port(s: Option<&str>) -> Option<u16> {
    s.and_then(|t| t.parse().ok()).filter(|&p| p >= 1024)
}
```

## The ? operator works on Option too

Inside a function that returns `Option`, `expr?` returns `None` early if `expr` is `None`, otherwise gives you the inner value:

```rust
fn first_char_upper(s: Option<&str>) -> Option<char> {
    let c = s?.chars().next()?;
    Some(c.to_ascii_uppercase())
}
```

## This lesson's exercise

Four small functions over a tiny user table. Use combinators, not nested `match`.
