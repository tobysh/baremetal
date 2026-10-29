---
title: Move Closures
summary: Why thread::spawn demands 'static closures, how move transfers ownership into a thread, and how to hand each thread its own data.
minutes: 20
difficulty: easy
tags: threads, closures, ownership
id: rust-concurrency/move-closures
mode: harness
---
Try to compile this and Rust refuses:

```rust
let name = String::from("Ada");
let h = thread::spawn(|| println!("hello, {name}"));
h.join().unwrap();
```

The error says the closure "may outlive the current function, but it borrows `name`". The spawned thread could keep running after this function returns and `name` is gone, which would be a dangling reference. `thread::spawn` therefore requires the closure to be `'static`: it may not borrow from the stack of its caller.

## The fix: move

Put `move` before the closure and it takes **ownership** of everything it uses:

```rust
let name = String::from("Ada");
let h = thread::spawn(move || println!("hello, {name}"));
```

The `String` now lives inside the closure, and therefore inside the thread. The price is that `name` is gone from the spawning side; using it afterwards is a "borrow of moved value" error. Types that are `Copy` (integers, chars) are copied instead, so `move || i * i` leaves your `i` usable.

## One value per thread

To give every thread its own piece of data, create or clone it inside the loop that spawns:

```rust
let handles: Vec<_> = names
    .into_iter()                       // consume the Vec, yielding owned Strings
    .map(|n| thread::spawn(move || n.len()))
    .collect();
```

If you need the original afterwards, `.clone()` before the `move`. Cloning a `String` per thread is cheap compared with fighting the borrow checker, and the next lessons show how to share one value instead.

## Returning a handle

Because a `'static` closure owns what it needs, a function can start a thread and hand the `JoinHandle` back to its caller, which joins later:

```rust
fn start(label: String) -> thread::JoinHandle<usize> {
    thread::spawn(move || label.len())
}
```

## This lesson's exercise

The starter functions all compile, but they are stubs. Fill them in so that every thread owns what it needs: `greet_all`, `indexed_squares` and `spawn_range_sum`.
