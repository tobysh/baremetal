---
title: References and borrowing
summary: Lend values with & and &mut, learn the one-writer-or-many-readers rule, and see why it makes data races and dangling pointers impossible.
minutes: 18
difficulty: medium
tags: references, borrowing
id: rust-foundations/references-and-borrowing
mode: harness
---
The last lesson introduced `&` as "lend without giving away". This lesson is about the rules behind it, because they are the part of Rust you will argue with most, and the part that pays you back most.

## Shared references

`&T` is a **shared reference**: read-only access to a `T` owned by someone else. You can have as many as you like at once.

```rust
let v = vec![1, 2, 3];
let a = &v;
let b = &v;
println!("{} {}", a.len(), b.len()); // fine: two readers
```

## Mutable references

`&mut T` is an **exclusive reference**: you may read and write, and nobody else may touch the value while it lives. The variable itself must be `mut` too.

```rust
fn add_one(n: &mut i32) {
    *n += 1;          // * follows the reference to the value
}

let mut x = 5;
add_one(&mut x);
println!("{}", x);    // 6
```

`*n` is the same dereference you know from C pointers. Method calls and field access dereference for you, so `v.push(4)` works on a `&mut Vec<i32>` without any star.

## The aliasing rule

At any moment a value may have **either**
- any number of `&` references, **or**
- exactly one `&mut` reference,

never both. The compiler checks this.

```rust
let mut v = vec![1, 2, 3];
let first = &v[0];       // shared borrow starts
v.push(4);               // error: needs &mut v while `first` is alive
println!("{}", first);   // shared borrow ends here
```

Why does Rust care? `push` may reallocate the vector, and then `first` would point at freed memory. In C that is a classic use-after-free that works on your machine and crashes in production. Rust refuses to compile it. The same rule rules out data races: a race needs two writers, or a writer and a reader, at once.

A borrow lasts only until its **last use**, not to the end of the block. If `first` were not printed after `v.push(4)`, the code above would compile.

## No dangling references

A reference can never outlive the value it points to:

```rust
fn bad() -> &String {            // error: missing lifetime / returns a reference to local
    let s = String::from("hi");
    &s                           // s is dropped when bad() returns
}
```

Return the `String` itself (give ownership away) instead.

## Returning a reference

A function can hand back a reference into something it was lent. With one reference parameter the compiler works out the link for you:

```rust
fn first_mut(v: &mut Vec<i32>) -> Option<&mut i32> {
    v.first_mut()
}

let mut data = vec![10, 20];
if let Some(x) = first_mut(&mut data) {
    *x += 5;             // writes straight into the vector
}
// data is now [15, 20]
```

While the returned reference is alive, `data` stays mutably borrowed.

## This lesson's exercise

Four small functions that each take their argument by the right kind of reference. The caller keeps ownership every time.
