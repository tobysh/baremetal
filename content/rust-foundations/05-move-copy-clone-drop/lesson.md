---
title: Move, Copy, Clone and Drop
summary: Know exactly when a value is moved, copied or cloned, and use Drop to run cleanup code the moment an owner goes away (RAII).
minutes: 20
difficulty: medium
tags: ownership, drop, traits
id: rust-foundations/move-copy-clone-drop
mode: harness
---
Passing a value around in Rust does one of three things. Knowing which saves you a lot of confusion.

## Move: the default

Assigning or passing a value **moves** it. The bytes are copied, but the old name becomes unusable, so only one owner ever frees the memory.

```rust
fn consume(s: String) { println!("{}", s); }   // s dropped at the end

let name = String::from("Ada");
consume(name);
// println!("{}", name);   // error: value moved
```

## Copy: tiny plain values

Types that are cheap and own nothing on the heap (`i32`, `f64`, `bool`, `char`, `&T`, tuples of those) implement the `Copy` trait. They are duplicated instead of moved, and the original stays valid.

```rust
let a = 5;
let b = a;
println!("{} {}", a, b);   // both fine
```

Your own types can opt in, as long as every field is `Copy`:

```rust
#[derive(Clone, Copy)]
struct Point { x: i32, y: i32 }
```

A type that owns heap memory, like `String` or `Vec`, can never be `Copy`: two owners would mean a double free.

## Clone: an explicit deep copy

When you really want a second independent value, ask for it with `.clone()`. It may allocate, which is why Rust makes you type it.

```rust
let a = String::from("hi");
let b = a.clone();         // new heap buffer
println!("{} {}", a, b);
```

Clone when you genuinely need two owners. Do not clone just to silence the borrow checker; a `&` is usually what you wanted.

## Drop: cleanup when the owner leaves

When an owner goes out of scope, Rust calls its `drop` method. You can implement the `Drop` trait to run your own code: close a file, release a lock, print a message. This pattern (acquire in the constructor, release in `drop`) is called **RAII**, and it replaces `free`, `fclose` and `unlock` calls you would forget in C.

```rust
struct Noisy(&'static str);

impl Drop for Noisy {
    fn drop(&mut self) {
        println!("dropping {}", self.0);
    }
}

fn main() {
    let _a = Noisy("a");
    let _b = Noisy("b");
    println!("end of main");
}
// prints: end of main, dropping b, dropping a
```

Locals are dropped in **reverse** order of creation. A value moved into a function is dropped when that function ends. You can drop early with `drop(value)`, which is just a function that takes its argument by value and lets it go out of scope.

## Logging drops in this exercise

To make drops observable, the starter has a `Guard` that holds a shared log: `Rc<RefCell<Vec<String>>>`. You do not need to master those types here. `Rc` is a shared-ownership handle (cloning it is cheap and points to the same log) and `RefCell` allows pushing to the log through a shared handle: `log.borrow_mut().push(text)`.

## This lesson's exercise

Implement `Drop` for `Guard`, and write three small functions that move and clone.
