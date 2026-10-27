---
title: Structs and methods
summary: Group data with struct, attach behavior with impl, and learn when a method needs &self or &mut self.
minutes: 18
difficulty: medium
id: rust-foundations/structs-and-methods
mode: harness
---
In C you grouped related values with a `struct` and wrote free functions that took a pointer to it. Rust keeps `struct`, and lets you attach functions to it directly.

```rust
struct Counter {
    count: u32,
    step: u32,
}
```

Create one by naming every field:

```rust
let c = Counter { count: 0, step: 2 };
println!("{}", c.count);
```

## impl blocks

Functions for a type live in an `impl` block:

```rust
impl Counter {
    // associated function: no self, called as Counter::new()
    fn new(step: u32) -> Counter {
        Counter { count: 0, step: step }
    }

    // method: first parameter is self
    fn get(&self) -> u32 {
        self.count
    }

    fn tick(&mut self) {
        self.count += self.step;
    }
}
```

An **associated function** like `new` has no `self` and is called with `::`. Rust has no special constructors; `new` is just a convention. A **method** takes `self` as its first parameter and is called with a dot.

## Three kinds of self

The first parameter says how the method uses the value, the same borrowing rules as before:

- `&self`: reads the value (borrows it immutably). Most methods.
- `&mut self`: modifies the value (borrows it mutably).
- `self`: takes ownership and consumes the value. Rare.

```rust
let mut c = Counter::new(5);
c.tick();            // requires `let mut`
println!("{}", c.get());
```

The common mistake is forgetting `mut` on the variable when calling a `&mut self` method. The error message says "cannot borrow `c` as mutable", and the fix is `let mut c`. Likewise, if a method only reads, use `&self`; don't ask for more access than you need.

## Self and field access

Inside `impl Counter`, you can write `Self` for `Counter`, and fields are reached through `self.field`. If a method's parameter has the same name as a field you can shorten `Counter { step: step }` to `Counter { step }`.

Methods can call each other: `self.len() == 0` is perfectly fine, and is how `is_empty` is usually written.

## This lesson's exercise

You will build a **stack**: last in, first out, like a pile of plates. It wraps a `Vec<i32>`, and `Vec` already has `push`, `pop` and `last`, so each of your methods is only a line or two. Note `pop` returns an `Option` because the stack might be empty.
