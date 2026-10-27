---
title: Box, Rc and RefCell
summary: Share and mutate data with smart pointers. Build a small tree whose nodes have several owners, using Rc and RefCell.
minutes: 30
difficulty: hard
id: rust-traits/smart-pointers
mode: harness
---
Ownership says every value has exactly one owner. Usually that is what you want. A **smart pointer** is a type that behaves like a pointer but adds a rule or a feature. Three are worth knowing now.

## Box<T>: one owner, on the heap

`Box<T>` is the closest thing to `malloc` in safe Rust: it puts a value on the heap, and frees it when the box goes out of scope. You already met `Box<dyn Trait>`. It is also how you build a type that contains itself, since a struct cannot hold itself directly (it would be infinitely big):

```rust
enum List {
    Cons(i32, Box<List>),
    Nil,
}
```

## Rc<T>: many owners

Sometimes a value really does have several owners, such as a node in a graph reachable from two parents. `Rc<T>` (reference counted) keeps a count of owners, and frees the value when the last one goes away. It is the equivalent of the manual reference count you may have written in C, but automatic.

```rust
use std::rc::Rc;

let a = Rc::new(5);
let b = Rc::clone(&a);          // cheap: bumps the count, copies no data
println!("{}", Rc::strong_count(&a));   // 2
```

An `Rc` gives only shared, read-only access. Plain `Rc` is for single-threaded code; threads use `Arc`.

## RefCell<T>: mutation checked at runtime

Rust's borrow rules (one writer *or* many readers) are normally checked at compile time. `RefCell<T>` moves the check to runtime, which lets you mutate through a shared handle:

```rust
use std::cell::RefCell;

let cell = RefCell::new(10);
*cell.borrow_mut() += 1;        // exclusive access, for this statement
println!("{}", cell.borrow());  // shared access: 11
```

If you break the rules (say, `borrow_mut` while a `borrow` is still alive), the program **panics** instead of failing to compile. So the combination to remember is **`Rc<RefCell<T>>`**: many owners, each able to change the value. Use it sparingly; most programs do not need it.

## A tree with shared nodes

```rust
struct Node {
    value: i32,
    children: Vec<Rc<RefCell<Node>>>,
}
```

To read a node you call `.borrow()`; to change one, `.borrow_mut()`. Recursing over children means borrowing each in turn. The common mistake is holding a `borrow_mut()` result in a variable while calling something that borrows the same node again, which panics. Keep borrows short.

One more caution: two `Rc`s pointing at each other would never reach count zero, a **reference cycle** that leaks. Trees where parents own children, and children do not own parents, are safe.

## This lesson's exercise

Build the helpers for a tree of shared nodes: create a node, attach a child (the child stays usable by its creator too, since it is shared), sum a whole tree, and double every value in place.
