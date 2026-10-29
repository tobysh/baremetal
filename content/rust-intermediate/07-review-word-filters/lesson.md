---
title: "Review: Word Filters"
summary: A consolidation exercise. Combine a trait, generics, trait objects and lifetimes to pick words out of a text without copying them. No new ideas.
minutes: 25
difficulty: medium
kind: review
tags: traits, lifetimes, trait-objects
id: rust-intermediate/review-word-filters
mode: harness
---
This is a **review lesson**. Nothing here is new; the point is to see traits and lifetimes cooperate.

It draws on [default methods](/learn/rust-intermediate/trait-defaults), [bounds and Iterator](/learn/rust-intermediate/generic-bounds-iterators), [trait objects](/learn/rust-intermediate/trait-object-pipeline) and [lifetimes in structs](/learn/rust-intermediate/lifetimes-in-structs).

## The pieces

```rust
trait Matcher {
    fn matches(&self, word: &str) -> bool;
}
```

- A **generic** function `select<'a, M: Matcher>(text: &'a str, m: &M)` is resolved at compile time for one matcher type.
- A **trait object** parameter `&[&dyn Matcher]` accepts a mix of matcher types in one list.
- The result is `Vec<&'a str>`: slices of the original text, tied to `text` and **not** to the matcher, so a matcher may be a temporary.

`Prefix<'p>` is a struct holding a borrowed prefix, so its `impl` needs a lifetime too: `impl Matcher for Prefix<'_>`.

## A closure is a matcher

The starter already contains a blanket implementation, so any closure `|w: &str| -> bool` is a `Matcher`:

```rust
impl<F: Fn(&str) -> bool> Matcher for F {
    fn matches(&self, word: &str) -> bool { self(word) }
}
```

You do not have to write it. Use it in your head to see why tests can pass closures wherever a `Matcher` is expected.
