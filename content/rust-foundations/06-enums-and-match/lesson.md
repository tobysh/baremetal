---
title: Enums and match
summary: Model "one of several things" with enums that carry data, and handle every case with exhaustive match.
minutes: 18
difficulty: medium
id: rust-foundations/enums-and-match
mode: harness
---
In C, an `enum` is just a list of named integers. When you needed "one of several shapes, each with different data", you reached for a `struct` with a tag field and a `union`, and nothing stopped you reading the wrong member. Rust enums solve this directly: each **variant** can carry its own data.

```rust
enum Message {
    Quit,                       // no data
    Move(i32, i32),             // two numbers
    Write(String),              // one String
}
```

A value of type `Message` is exactly one of these variants, and the data comes with it.

```rust
let m = Message::Move(3, 4);
```

## match

You get at the data with `match`, which compares a value against **patterns**:

```rust
fn describe(m: &Message) -> String {
    match m {
        Message::Quit => String::from("quit"),
        Message::Move(x, y) => format!("move to {}, {}", x, y),
        Message::Write(text) => format!("write {}", text),
    }
}
```

Patterns like `Move(x, y)` pull the data out into new variables. `match` is an expression, so the whole thing produces a value (here a `String`; `format!` works like `println!` but returns the text).

## Exhaustive

The compiler insists that `match` covers **every** variant. Delete the `Quit` line above and it will not compile: "non-exhaustive patterns: `&Message::Quit` not covered". Later, if you add a variant to the enum, the compiler shows you every `match` that needs updating. That is a bug class C compilers cannot catch.

A catch-all pattern `_ => ...` matches anything left. The common mistake is reaching for `_` too early; it silences exactly the safety net described above. Use it only when you truly mean "everything else".

## Option is an enum

`Option` is not magic. It is defined in the standard library as:

```rust
enum Option<T> {
    None,
    Some(T),
}
```

That is why `match` on an `Option` makes you handle `None`. You can also match numbers and ranges (`0..=9 => ...`), and add conditions with `if` ("guards").

## Matching on references

When a function takes `&Shape`, matching `Shape::Circle(r)` gives `r` as a reference to the number. Rust usually lets you use it in arithmetic directly; if the compiler complains, write `*r` to get the value itself.

For the exercise, **Heron's formula** gives a triangle's area from its side lengths: let `s = (a + b + c) / 2`, then the area is the square root of `s * (s - a) * (s - b) * (s - c)`. In Rust, `x.sqrt()` on an `f64` is the square root.
