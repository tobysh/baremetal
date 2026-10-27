---
title: Closures
summary: Use closures, anonymous functions that capture their surroundings, and the Fn, FnMut and FnOnce traits that describe them.
minutes: 20
difficulty: medium
tags: closures
id: rust-traits/closures
mode: harness
---
In C, to pass behaviour around you pass a function pointer, and any extra data goes through a separate `void *context` argument you must manage by hand. A Rust **closure** is an anonymous function that can capture variables from where it was written, so the "context" travels with it automatically.

```rust
let bonus = 5;
let add_bonus = |x| x + bonus;   // captures `bonus`
println!("{}", add_bonus(10));   // 15
```

The `|x|` part is the parameter list; the rest is the body. Types are usually inferred. You have already used closures with iterators: `.map(|x| x * 2)`.

## Three traits: Fn, FnMut, FnOnce

Closures differ in what they do with what they capture, and Rust gives each kind a trait:

- **`Fn`**: only reads its captures. Can be called many times, even at once.
- **`FnMut`**: modifies its captures. Needs `mut` to call.
- **`FnOnce`**: moves a capture out, so it can only be called once.

Every closure that can be called more than once is also `FnOnce`, and every `Fn` is also `FnMut`. When you accept a closure, ask for the weakest trait you can use, which keeps your function the most flexible.

```rust
fn apply<F: Fn(i32) -> i32>(f: F, x: i32) -> i32 {
    f(x)
}
```

`F: Fn(i32) -> i32` is a trait bound, just like in the generics lesson.

## Returning closures and move

Each closure has its own unnameable type, so to return one you write `impl Fn...`. A returned closure must own its captures, because the local variables vanish when the function ends. The `move` keyword makes the closure take ownership:

```rust
fn make_adder(n: i32) -> impl Fn(i32) -> i32 {
    move |x| x + n
}
```

Without `move`, the closure would borrow `n`, and the compiler would reject it: "closure may outlive the current function". The fix it suggests is exactly `move`.

A stateful closure keeps its own private variable:

```rust
fn make_counter() -> impl FnMut() -> u32 {
    let mut count = 0;
    move || { count += 1; count }
}
```

## This lesson's exercise

Write `make_adder`, `apply_n` (apply a function repeatedly) and `make_counter`.
