---
title: Trait objects vs generics
summary: Choose between generics (static dispatch) and dyn Trait (dynamic dispatch), and keep mixed collections with Box<dyn Trait>.
minutes: 20
difficulty: medium
tags: trait-objects, generics
id: rust-traits/trait-objects
mode: harness
---
[Last lesson](/learn/rust-traits/traits-and-impl), `fn total_area<T: Shape>(shapes: &[T])` worked for a slice of squares **or** a slice of rectangles, but never a mix. A `Vec<T>` holds one type, and all elements of an array must be the same size. What if you want a list containing both?

That is what a **trait object** is for.

## Two ways to take "anything with this trait"

```rust
// generic: the compiler makes one copy per type
fn speak_generic<T: Animal>(a: &T) -> String { a.sound() }

// trait object: one copy, the method is looked up at runtime
fn speak_dyn(a: &dyn Animal) -> String { a.sound() }
```

- **Generics** use *static dispatch*. The compiler knows the exact type, can inline the call and optimise heavily. The cost is code size, since each type gets its own copy, and every element in a collection must share one type.
- **`dyn Trait`** uses *dynamic dispatch*. A reference to a trait object is a **fat pointer**: one pointer to the data plus one to a table of the type's methods (a *vtable*, just like the function-pointer struct you would build by hand in C). The call goes through the table, a small runtime cost, but it lets different types live behind the same name.

Rule of thumb: start with generics. Reach for `dyn` when you need a mixed collection, or when the concrete type is only known at runtime.

## Box<dyn Trait>

A `dyn Trait` has no known size, so you cannot hold one directly. Put it behind a pointer. `&dyn Trait` borrows one, and `Box<dyn Trait>` owns one on the heap:

```rust
let zoo: Vec<Box<dyn Animal>> = vec![Box::new(Dog), Box::new(Cat)];
for a in &zoo {
    println!("{}", a.sound());
}
```

`Box::new(Dog)` has type `Box<Dog>`; the annotation on `zoo` makes Rust coerce each element to `Box<dyn Animal>`.

## Object safety

Not every trait can become a `dyn`. A method that returns `Self`, or has its own generic parameters, cannot be called through a vtable, because the compiler would not know the concrete type. If you see "the trait cannot be made into an object", look for one of those.

## This lesson's exercise

You will implement `Animal` for `Dog` and `Cat`, then write functions that take it both ways: `speak_generic` (static) and `speak_dyn` (dynamic), and `chorus`, which takes a slice of boxed animals of mixed kinds and joins what each one says.
