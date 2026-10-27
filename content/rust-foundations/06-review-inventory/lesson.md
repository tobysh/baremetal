---
title: "Review: Inventory"
summary: A consolidation exercise. Combine structs, owned strings, vectors, iterators and Option in a small stock-keeping type. No new ideas, just practice.
minutes: 25
difficulty: medium
kind: review
tags: structs, ownership, iterators
id: rust-foundations/review-inventory
mode: harness
---
This is a **review lesson**. It introduces nothing new. By now you have met ownership and `String`, `Vec` and iterators, and structs with methods. A real Rust program uses all of them in the same breath, so this exercise does too.

Lessons this review draws on: [Ownership and strings](/learn/rust-foundations/ownership-and-strings), [Vectors and iterators](/learn/rust-foundations/vectors-and-iterators), [Structs and methods](/learn/rust-foundations/structs-and-methods).

## What you already know

- **Owned and borrowed strings:** a `String` owns its text, a `&str` borrows it. A function that only reads text should take `&str`, and a struct that keeps text needs a `String` (`name.to_string()` makes one).
- **Methods and `self`:** `&self` reads, `&mut self` changes, and `Type::new()` is just an ordinary function that returns a value.
- **Iterators:** `.iter()` borrows each item, `.iter_mut()` lets you change each one, and adapters such as `filter`, `map`, `find` and `sum` chain together. `find` returns an `Option`.
- **Option:** `Some(x)` or `None`, handled with `match`, `if let`, or helpers like `map` and `unwrap_or`.

## The shape of the problem

An `Inventory` holds a `Vec<Item>`, and each `Item` has a `name: String` and a `qty: u32`. You will write six methods. They fall into three patterns you have already seen:

1. **Look something up**: `count` finds an item by name and gives back its quantity, or `0` when there is no such item. That is `iter().find(...)` followed by `map` and `unwrap_or`.
2. **Change something that exists, or create it**: `add` looks for an item with `iter_mut().find(...)`. If it finds one, it increases `qty`. Otherwise it pushes a new `Item`. This is the classic `if let Some(..) = .. { .. } else { .. }`.
3. **Summarise everything**: `total` sums the quantities, and `in_stock` collects the names of the items whose quantity is above zero.

`remove` mixes the second and first: it only succeeds if the item exists **and** has enough stock, otherwise it must leave everything unchanged and return `false`.

Items stay in the list once added, even after their quantity reaches zero, and they stay in the order they were first added. The borrow checker will push back if you try to hold a reference into the `Vec` while pushing to it. If it complains, finish using the reference before you push, which is what the `if let ... else` shape achieves.
