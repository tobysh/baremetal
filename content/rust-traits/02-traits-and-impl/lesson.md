---
title: Traits and impl
summary: Define shared behaviour with a trait, implement it for your own types, and use default methods and trait-bounded functions.
minutes: 16
difficulty: easy
tags: traits
id: rust-traits/traits-and-impl
mode: harness
---
A **trait** is a list of methods that a type promises to provide. If you know C, think of a struct of function pointers, except the compiler wires it up and checks it for you. If you have met interfaces elsewhere, it is the same idea.

```rust
trait Greet {
    fn name(&self) -> String;

    // a default method: types get it for free
    fn hello(&self) -> String {
        format!("Hello, {}!", self.name())
    }
}
```

A method with no body must be written by each implementer. A method with a body is a **default**, which implementers may keep or replace.

## Implementing a trait

```rust
struct Knight;
struct Mage { title: String }

impl Greet for Knight {
    fn name(&self) -> String {
        String::from("brave knight")
    }
}

impl Greet for Mage {
    fn name(&self) -> String {
        format!("{} the Mage", self.title)
    }
}
```

`impl Trait for Type` gives that type the methods. Now `Knight.hello()` prints "Hello, brave knight!" without `Knight` having written `hello` itself.

## Using traits in functions

Combine this with last lesson's bounds. A function can accept any type that implements a trait:

```rust
fn welcome<T: Greet>(who: &T) -> String {
    who.hello()
}
```

You will often see the shorter spelling `fn welcome(who: &impl Greet)`, which means the same thing.

Rust has one rule worth remembering (the **orphan rule**): you may implement a trait only if you own either the trait or the type. You can implement your own trait for `i32`, and the standard `Display` trait for your own struct, but not `Display` for `i32`.

## This lesson's exercise

You will give two shapes, `Square` and `Rectangle`, the trait `Shape`. The trait has a default `describe` method already written; your job is to implement `area` and `perimeter` for each shape, plus `name`, and a generic `total_area` that adds up the area of a slice of one kind of shape.
