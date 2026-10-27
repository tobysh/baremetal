---
title: Generics
summary: Write one function or struct that works for many types, using type parameters and trait bounds like PartialOrd and Copy.
minutes: 15
difficulty: easy
id: rust-traits/generics
mode: harness
---
In C, a function that finds the largest `int` cannot also find the largest `double`. You either copy it, or reach for `void *` and give up type checking. Rust has a third way: **generics**. You write the function once, with a placeholder for the type, and the compiler builds a separate, fully optimised copy for each type you actually use. There is no runtime cost.

```rust
fn first<T>(items: &[T]) -> Option<&T> {
    items.first()
}
```

`<T>` introduces a **type parameter**. Read it as "for any type, call it T". Callers never write `T`; Rust infers it: `first(&[1, 2, 3])` uses `T = i32`.

## Trait bounds

A bare `T` could be anything, so you can do almost nothing with it. You cannot compare two `T`s, because not every type can be compared. To say what you need, add a **bound**:

```rust
fn bigger<T: PartialOrd>(a: T, b: T) -> T {
    if a > b { a } else { b }
}
```

`T: PartialOrd` means "any type that supports `<` and `>`". Several bounds are joined with `+`: `T: PartialOrd + Copy`. `Copy` means the value can be duplicated by a simple bit copy, as integers and chars can. That lets you pull a value out of a slice without taking ownership.

The common mistake is leaving a bound out. The compiler's message is a friendly one: "binary operation `>` cannot be applied to type `T`", with a `help:` that suggests adding `T: PartialOrd`.

## Generic structs

Types can have parameters too, as `Vec<T>` and `Option<T>` do:

```rust
struct Pair<T> {
    first: T,
    second: T,
}

impl<T> Pair<T> {
    fn new(first: T, second: T) -> Pair<T> {
        Pair { first, second }
    }
}
```

Note `impl<T>`: you declare the parameter after `impl`, then use it in `Pair<T>`.

## This lesson's exercise

You will write `largest`, which returns the biggest item of a slice (or `None` if the slice is empty), and give `Pair` a `swap` method. Both must work for integers, floats and chars alike.
