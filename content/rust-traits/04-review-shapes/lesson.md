---
title: "Review: Shape Collection"
summary: A consolidation exercise. Combine trait implementations, a default method, generics and trait objects around one small Shape trait. No new ideas.
minutes: 25
difficulty: medium
kind: review
tags: traits, generics, trait-objects, review
id: rust-traits/review-shapes
mode: harness
---
This is a **review lesson**. It introduces nothing new. You have seen generics, traits, and trait objects separately, and the useful thing is to notice how they fit together around a single trait.

## What you already know

- **A trait** is a set of methods a type promises to have. A trait can also provide a **default method**, written in the trait itself, that every implementor gets for free and may override.
- **Generics with a bound** (`fn f<T: Shape>(items: &[T])`) are resolved at compile time. Every item in the slice is the *same* type.
- **Trait objects** (`Box<dyn Shape>`, `&dyn Shape`) are resolved at run time. That is what lets one `Vec` hold squares, circles and rectangles side by side.

## The trait

The starter defines the trait for you:

```rust
trait Shape {
    fn area(&self) -> f64;
    fn name(&self) -> String;
    fn summary(&self) -> String {
        format!("{} (area {:.1})", self.name(), self.area())
    }
}
```

`area` and `name` are required, but `summary` is a default method built on top of them. You do **not** write `summary` yourself: implementing the two required methods on a type is enough for `summary` to work.

## What to write

1. **Implement the trait three times**, for `Square`, `Rect` and `Circle`. The names are `"square"`, `"rect"` and `"circle"`, and the area formulas are the ones you remember (use `std::f64::consts::PI`).
2. **`total_area`** is generic: it accepts a slice of any one shape type and adds the areas up. The compiler builds a separate version for each type you call it with.
3. **`describe_largest`** takes a slice of boxed trait objects, so the shapes can all be different kinds. Find the one with the greatest area and return its `summary()`, or `None` for an empty slice. `f64` is not `Ord`, so `max()` will not compile; use `max_by` with `partial_cmp`.
4. **`report`** joins every shape's `summary()` with `"; "`, again over trait objects.

Same trait, two ways of using it. If you are unsure which to reach for, ask: "will all the items be the same type?" If yes, a generic is fine. If not, you need a trait object.
