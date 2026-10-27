---
title: Ownership and strings
summary: Understand ownership, moves and borrowing, the idea that replaces malloc and free, and the difference between String and &str.
minutes: 20
difficulty: medium
id: rust-foundations/ownership-and-strings
mode: harness
---
In C you called `malloc` and had to remember `free`. Forget it and you leak; do it twice or use the memory afterwards and you get undefined behavior. Rust removes this by giving every value exactly one **owner**, and freeing the value automatically when the owner goes out of scope. There is no garbage collector: the compiler works out where to free at compile time.

## Three rules

1. Every value has one owner (a variable).
2. There is only one owner at a time.
3. When the owner goes out of scope, the value is dropped (freed).

## Moves

```rust
let a = String::from("hi");
let b = a;          // ownership MOVES from a to b
println!("{}", a);  // error: borrow of moved value `a`
```

Copying a `String` would mean copying heap memory, so Rust moves it instead and invalidates `a`. This is what stops a double free. Small plain values like `i32` are simply copied, so they don't move.

## Borrowing

Often you just want to *look at* a value. Pass a **reference** with `&`, which borrows without taking ownership:

```rust
fn length(s: &String) -> usize {
    s.len()
}

let a = String::from("hi");
let n = length(&a);   // a is only lent
println!("{} {}", a, n); // still fine
```

A reference is like a C pointer that the compiler guarantees is never dangling or null. The rule: you may have many `&` (read-only) borrows *or* one `&mut` (writable) borrow at a time, never both. The common mistake is fighting the compiler with extra `.clone()` calls; usually you just need to borrow instead.

## String vs &str

- `String` is an owned, growable piece of text on the heap.
- `&str` (a "string slice") is a borrowed view of some text, such as a literal `"hello"` or part of a `String`.

Prefer `&str` for parameters: it accepts both. A function can return a slice of its input, and the compiler checks the input outlives it:

```rust
fn first_char_slice(s: &str) -> &str {
    &s[0..1]
}
```

## Text is UTF-8

Strings are UTF-8, so one visible character can take 1 to 4 bytes. `"é"` is 2 bytes. That is why you cannot index with `s[i]`, and why `len()` counts **bytes**, not characters. Use `chars()` to walk real characters:

```rust
let s = "héllo";
println!("{}", s.len());           // 6 bytes
println!("{}", s.chars().count()); // 5 characters
```

Slicing with `&s[0..2]` must land on a character boundary or the program panics. To find a boundary, `s.find(' ')` returns the byte position of a space, which is always safe to slice at, because a space is a single byte.
