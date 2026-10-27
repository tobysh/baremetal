---
title: Modules and Visibility
summary: Group code into mod blocks, decide what is pub and what stays private, and reach items with use and paths like super::.
minutes: 15
difficulty: easy
id: rust-practice/modules-visibility
mode: harness
---
A **module** is a named box for code. Real projects put modules in separate files, but a `mod` block in one file works the same way, and it is the easiest way to learn the rules.

```rust
mod garden {
    pub struct Plot {
        pub name: String,
        water: u32,      // private
    }

    impl Plot {
        pub fn new(name: &str) -> Plot {
            Plot { name: name.to_string(), water: 0 }
        }

        pub fn water(&mut self) {
            self.water = bump(self.water);
        }

        pub fn level(&self) -> u32 {
            self.water
        }
    }

    fn bump(n: u32) -> u32 { n + 1 }   // private helper
}
```

## Private by default

Everything inside a module is private to that module (and its children) unless you mark it `pub`. Outside code can call `garden::Plot::new` but not `garden::bump`, and cannot write `plot.water`, because the field is private. This is the main reason to use modules: the **public** items are the promise you make; the private ones are free to change.

Making a struct `pub` does not make its fields `pub`. Each field decides for itself. A struct with a private field cannot be built with `Plot { ... }` from outside, so callers are forced through your `new`, and your rules (a counter that never goes backwards, a size that is never zero) cannot be bypassed.

## Paths and use

Reach an item by its path: `garden::Plot::new("herbs")`. A `use` line brings a name into scope so you can write it short:

```rust
use garden::Plot;
```

Inside a module, `super::` means "the module above me", and `self::` means "this one". Child modules can see their parents' private items, but never the other way round.

## This lesson's exercise

Fill in a `counter` module with a private field and a private helper, and a nested public `format` module that reads from its parent.
