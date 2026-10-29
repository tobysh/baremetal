---
title: Reading and Writing Lifetime Annotations
summary: Learn when the compiler can guess lifetimes (elision), when you must name them, and how separate lifetimes keep one argument from constraining another.
minutes: 30
difficulty: medium
tags: lifetimes, references, strings
id: rust-intermediate/lifetime-annotations
mode: harness
---
Every reference has a **lifetime**: the stretch of code during which it is valid. Usually the compiler works it out. A lifetime annotation does not make anything live longer; it *describes* how the lifetimes of the inputs and the output relate, so the compiler can check the promise.

## Elision: the compiler fills it in

With one input reference, the output must come from it, so you can omit the names:

```rust
fn first_word(s: &str) -> &str { /* ... */ }
// means: fn first_word<'a>(s: &'a str) -> &'a str
```

With two or more input references, the compiler cannot tell which one the result borrows from, and it asks you:

```rust
fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    if b.len() > a.len() { b } else { a }
}
```

`'a` says: the result is valid only as long as **both** arguments are. The result might be either one, so it must not outlive either.

## Several lifetimes

If the result comes only from one argument, say so. Then the other argument may be dropped while the result lives on:

```rust
fn strip_known<'a>(text: &'a str, prefix: &str) -> &'a str {
    text.strip_prefix(prefix).unwrap_or(text)
}
```

`prefix` has its own anonymous lifetime. Nothing in the output borrows from it, so a temporary prefix is fine. If you wrote `prefix: &'a str` too, the compiler would force `prefix` to live as long as the result, and callers with a short-lived prefix would get an error.

## Slices of the input

`&str` slicing (`&s[..i]`) produces a reference into the same memory, so the slice has the same lifetime as `s`. That is why `split_pair` can return two pieces of the input without copying.

## This lesson's exercise

The starter gives the signatures. Write the bodies, and notice why each lifetime annotation is shaped the way it is: the hidden tests include a case that only compiles when `strip_known` keeps its two arguments' lifetimes separate.
