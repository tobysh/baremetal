---
title: Hello, Rust
summary: Meet Rust, the language that gives C-level control with compiler-checked memory safety. Write main, println!, let and mut.
minutes: 12
difficulty: easy
id: rust-foundations/hello-rust
mode: stdout
---
In C you were trusted with everything: you could read past the end of an array, use memory after freeing it, or forget to free it at all. The compiler said nothing, and the program crashed later (or worse, kept running with corrupted data). Most serious security bugs in C and C++ programs are exactly this kind of **memory-safety** bug.

**Rust** is a systems language, just as fast and low-level as C, with no garbage collector and no hidden runtime. The difference is that the compiler checks your memory use *before* the program runs. If it can't prove your code is safe, it refuses to build it. That feels strict at first, but the errors are some of the most helpful you will ever read.

## The smallest Rust program

```rust
fn main() {
    println!("Hi!");
}
```

- `fn` declares a function. `main` is where every program starts, as in C, but it takes no `void` and needs no `return 0`.
- `println!` ends with `!` because it is a **macro**, not a normal function. It prints a line and adds the newline for you.
- There are no `#include`s. The common parts of the standard library are already available.

## Variables: let and mut

```rust
let year = 2021;
let name = "Rust";
println!("{} since {}", name, year);
```

`{}` is a placeholder, filled in by the arguments in order (like `%d` and `%s` in `printf`, but one placeholder works for any printable type). You can also write the variable name inside: `println!("{name} since {year}")`.

You never wrote a type. Rust uses **type inference**: it works out that `year` is an integer from the value. The type is still fixed at compile time, just like C.

Here is the big surprise. Variables are **immutable** by default:

```rust
let count = 1;
count = 2; // error: cannot assign twice to immutable variable
```

To allow change, say so with `mut`:

```rust
let mut count = 1;
count = 2; // fine
```

The common mistake is adding `mut` to everything to silence errors. Use it only when a value really changes. Knowing at a glance which values never change makes code much easier to read.

## The compiler is a teacher

Rust errors name the problem, point at the exact spot, and often suggest the fix:

```
error[E0384]: cannot assign twice to immutable variable `count`
help: consider making this binding mutable: `mut count`
```

Read the whole message, not just the first line. The `help:` part is frequently the answer.
