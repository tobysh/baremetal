---
title: A Pipeline of Trait Objects
summary: Store different types behind Box<dyn Trait>, build them from text, and run a value through them with ? short-circuiting on Option.
minutes: 30
difficulty: medium
tags: traits, trait-objects, option
id: rust-intermediate/trait-object-pipeline
mode: harness
---
A generic `Vec<T>` holds one type. To store a mix of types that share behaviour, use a **trait object**: `Box<dyn Stage>` is an owned pointer to *some* value that implements `Stage`, and calls go through a table of function pointers at run time.

```rust
trait Stage {
    fn name(&self) -> String;
    fn apply(&self, x: i64) -> Option<i64>;
}

let stages: Vec<Box<dyn Stage>> = vec![Box::new(AddN(5)), Box::new(NonNeg)];
```

## Building boxes from data

A function that returns different concrete types depending on its input cannot return a plain type, but it can return `Box<dyn Stage>`. Wrapping with `Box::new(...)` coerces the concrete box into the trait object:

```rust
fn build(spec: &str) -> Option<Box<dyn Stage>> {
    match spec {
        "nonneg" => Some(Box::new(NonNeg)),
        _ => None,
    }
}
```

## `?` on Option

Inside a function that returns `Option`, `expr?` yields the inner value or returns `None` straight away. That makes a chain of fallible steps read like a straight line:

```rust
let mut v = x;
for s in stages { v = s.apply(v)?; }
Some(v)
```

Collecting an iterator of `Option<T>` into `Option<Vec<T>>` gives `None` as soon as one item is `None`; the same trick works with `Result`.

## Checked arithmetic

`a.checked_add(b)` returns `None` on overflow instead of panicking or wrapping. Stages built on it fail cleanly with huge inputs.

## This lesson's exercise

The starter defines four stages. Fill in their `name` and `apply`, then write `build`, `build_all`, `run` and `names`.
