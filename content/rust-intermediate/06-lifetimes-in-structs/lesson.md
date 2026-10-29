---
title: Lifetimes in Structs
summary: Store references inside a struct, write impl blocks with lifetime parameters, and build a zero-copy tokenizer and record parser that borrow from their input.
minutes: 35
difficulty: medium
tags: lifetimes, structs, iterators
id: rust-intermediate/lifetimes-in-structs
mode: harness
---
A struct that holds a reference must say how long the reference lasts:

```rust
struct Record<'a> {
    key: &'a str,
    value: &'a str,
}
```

`Record<'a>` cannot outlive the text it points into. The compiler checks this wherever a `Record` is used. The payoff is **zero-copy parsing**: instead of allocating a `String` for every field, you keep slices of the original text.

## impl blocks

The lifetime is a parameter of the type, so the `impl` declares it and passes it on:

```rust
impl<'a> Tokens<'a> {
    fn new(text: &'a str) -> Self { Tokens { rest: text } }
}
```

## Iterators that hand out borrowed items

An iterator's `Item` can be a reference with the struct's lifetime, **not** the lifetime of the `&mut self` borrow in `next`:

```rust
impl<'a> Iterator for Tokens<'a> {
    type Item = &'a str;
    fn next(&mut self) -> Option<&'a str> { /* ... */ }
}
```

This matters: because the items are `&'a str`, you can `collect()` them into a `Vec` and keep using them after the iterator is gone. If they were tied to the `&mut self` borrow, you could never hold two at once.

## Returning part of a borrowed collection

`fn longest_value<'a>(records: &[Record<'a>]) -> Option<&'a str>` returns a `&'a str`, not a reference into the slice. The slice itself may be dropped; the text it pointed into is what must stay alive.

## This lesson's exercise

The starter has the struct definitions and annotated signatures. Implement `Tokens::next` (split on whitespace by hand, without `split_whitespace`), `parse_record`, `parse_all` and `longest_value`.
