---
title: Operator and Formatting Traits
summary: Make your own type work with +, -, *, unary minus, +=, sum() and println! by implementing Add, Sub, Mul, Neg, AddAssign, Sum and Display.
minutes: 25
difficulty: easy
tags: traits, operators, display
id: rust-intermediate/operator-traits
mode: harness
---
In Rust, `a + b` is sugar for `Add::add(a, b)`. Every operator is a trait in `std::ops`, so your own types can use operators the same way numbers do.

```rust
use std::ops::Add;

#[derive(Debug, Clone, Copy, PartialEq)]
struct Point { x: i32, y: i32 }

impl Add for Point {
    type Output = Point;                       // what `a + b` produces
    fn add(self, o: Point) -> Point { Point { x: self.x + o.x, y: self.y + o.y } }
}
```

`type Output` is an **associated type**: each implementor chooses what the operation returns. `Mul<i64>` multiplies by a plain number rather than by another `Cents`: the trait is generic over the right-hand type, so a type can have several `Mul` impls.

## Other useful traits

- `Neg` is the unary `-x`. `Sub` is `a - b`.
- `AddAssign` is `a += b`; its method takes `&mut self` and returns nothing.
- `Display` is what `{}` prints. `fmt(&self, f: &mut fmt::Formatter) -> fmt::Result`, and you `write!(f, ...)` into `f`. Once `Display` exists, `to_string()` works for free.
- `Sum` lets `iterator.sum::<Cents>()` work. Implement `fn sum<I: Iterator<Item = Cents>>(iter: I) -> Cents`.
- `PartialOrd`/`Ord` can be derived; fields are compared in order.

## Formatting numbers

`{:02}` pads an integer with zeros to width 2, so `5` prints `05`. `write!(f, "{sign}${}.{:02}", a / 100, a % 100)` prints `$12.05`. The `$` there is just a character.

## This lesson's exercise

The starter has `Cents`, a money amount stored as a whole number of cents. Implement `Add`, `Sub`, `Mul<i64>`, `Neg`, `AddAssign`, `Sum` and `Display` so the tests can use `Cents` like a number.
