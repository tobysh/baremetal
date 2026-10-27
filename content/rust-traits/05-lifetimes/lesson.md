---
title: Lifetimes
summary: Learn what 'a means, how references are checked against the data they borrow, and how to store references in structs.
minutes: 25
difficulty: hard
tags: lifetimes, borrowing
id: rust-traits/lifetimes
mode: harness
---
In C, returning a pointer to a local variable compiles happily and fails later. Rust prevents this with **lifetimes**: every reference is only valid for some stretch of the program, and the compiler checks that no reference outlives the data it points to.

Most of the time you never write a lifetime, because Rust fills them in. You must write them when the compiler cannot tell which input a returned reference came from.

## Why functions need annotations

```rust
fn longest(a: &str, b: &str) -> &str {   // error: missing lifetime specifier
    if a.len() >= b.len() { a } else { b }
}
```

The result borrows from *either* `a` or `b`. The compiler needs to know how long the result stays valid, so you tell it:

```rust
fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    if a.len() >= b.len() { a } else { b }
}
```

`'a` is a lifetime parameter, declared in angle brackets like a generic type. The signature says: "the result is valid for as long as **both** inputs are". Lifetimes never change how long anything lives; they only describe relationships so the compiler can check them.

## Elision: the rules that let you skip them

If a function has exactly one reference input, the output borrows from it, so `fn first_word(s: &str) -> &str` needs no annotation. For methods, if there is a `&self`, outputs borrow from `self`. Only the ambiguous cases need `'a`.

## References inside structs

A struct holding a reference must say how long that reference lives:

```rust
struct Excerpt<'a> {
    text: &'a str,
}

impl<'a> Excerpt<'a> {
    fn new(text: &'a str) -> Excerpt<'a> { Excerpt { text } }
}
```

The struct cannot outlive the text it points at. If the original `String` is dropped first, the compiler stops you. This is exactly the dangling-pointer bug from C, caught at compile time.

## A parser that hands out slices

A powerful pattern: a struct keeps a `&'a str` and returns slices of it, with **no copying**. Note the return type below uses `'a` (the lifetime of the original text), not the lifetime of the `&mut self` borrow, which means the words you got earlier stay usable while the parser moves on:

```rust
struct Words<'a> { rest: &'a str }

impl<'a> Words<'a> {
    fn next_word(&mut self) -> Option<&'a str> {
        // ...
    }
}
```

## This lesson's exercise

Write `longest`, `first_word`, and a `Words` parser. `next_word` should skip leading spaces, return the next run of non-space characters, and advance `rest` past it. Useful tools: `trim_start()`, `find(' ')` and `split_at(i)`, all of which return slices of the original text.
