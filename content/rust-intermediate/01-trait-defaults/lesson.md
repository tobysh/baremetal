---
title: Default Methods and Associated Consts
summary: Write a trait with required and default methods, give it an associated constant, and let each type implement only what is unique to it.
minutes: 20
difficulty: easy
tags: traits, default-methods, associated-consts
id: rust-intermediate/trait-defaults
mode: harness
---
A trait names behaviour. You have already implemented traits; now look at how a good trait is shaped. Most of the methods should not need to be written by every implementor.

```rust
trait Greeter {
    const GREETING: &'static str;          // associated constant
    fn name(&self) -> String;              // required: every type writes this
    fn greet(&self) -> String {            // default: written once, here
        format!("{}, {}", Self::GREETING, self.name())
    }
}
```

## Required and default methods

A method with no body is **required**. A method with a body is a **default**; implementors get it for free and may override it. A default method can call the required ones, so one small piece of type-specific code unlocks a whole family of behaviour. This is how `Iterator` works: you write `next`, and you receive `map`, `filter`, `sum` and about seventy others.

## Associated constants

A trait can declare a `const` that every implementor fills in. Inside the trait use `Self::NAME`; outside use `LengthScorer::NAME`. Constants are known at compile time, so there is no run-time cost. A trait with an associated constant cannot be made into a `dyn` object, which is a trade-off to remember.

```rust
impl Greeter for Person {
    const GREETING: &'static str = "hello";
    fn name(&self) -> String { self.0.clone() }
}
```

## This lesson's exercise

The starter declares a `Scorer` trait that rates words, three scorers, and stubs for the default methods. Write the three default methods in the trait (`total`, `best`, `label`) and the `score` method for each scorer. The default methods must only use `score` and `Self::NAME`, so they work for every scorer, including ones that do not exist yet.
