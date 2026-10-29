---
title: Returning Iterators from Functions
summary: Return impl Iterator or Box<dyn Iterator> so callers get a lazy pipeline without a collected Vec in between.
minutes: 20
difficulty: medium
tags: iterators, impl-trait, laziness
id: rust-bridge/returning-iterators
mode: harness
---
A function that builds a pipeline often ends with `.collect::<Vec<_>>()`. That allocates, and it does all the work immediately even if the caller only wants the first few items. Better: return the iterator itself.

The type of a chain like `(0..).map(|x| x * x).filter(...)` is a long, unnameable mouthful (closures have anonymous types). Rust lets you say only what matters:

```rust
fn squares() -> impl Iterator<Item = u64> {
    (1..).map(|x| x * x)
}

let first: Vec<u64> = squares().take(4).collect(); // [1, 4, 9, 16]
```

`impl Iterator<Item = u64>` means "some concrete iterator type that yields `u64`; the compiler knows which, you only see the trait". It costs nothing at run time.

## Taking a closure and moving data in

If the returned iterator uses a parameter, the closure must **own** it, so write `move`:

```rust
fn multiples_of(k: u32) -> impl Iterator<Item = u32> {
    (1..).map(move |i| i * k)
}
```

## When the branches differ: Box<dyn Iterator>

`impl Trait` stands for exactly one type. If two `if` branches build different iterator types, they cannot both be the return type. Box them as a trait object:

```rust
fn evens_or_all(only_even: bool) -> Box<dyn Iterator<Item = u32>> {
    if only_even { Box::new((0..).step_by(2)) } else { Box::new(0..) }
}
```

You pay one heap allocation and a dynamic call per item, which is fine when you need the flexibility.

## This lesson's exercise

All the returned iterators must be lazy; some of them are infinite, and the tests would hang if you collected them.
