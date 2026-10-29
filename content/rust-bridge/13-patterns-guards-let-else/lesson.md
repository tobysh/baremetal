---
title: Guards, Bindings and let-else
summary: Combine match guards, @ bindings, ranges, nested destructuring and let-else to write flat, readable control flow.
minutes: 20
difficulty: medium
tags: pattern-matching, let-else, guards
id: rust-bridge/patterns-guards-let-else
mode: harness
---
`match` is more than a switch. A pattern can test, destructure and name parts of a value at once.

## Ranges, alternatives and guards

```rust
match score {
    90..=100 => 'A',
    80..=89 => 'B',
    0..=79 => 'C',
    _ => '?',
}

match n {
    1 | 2 | 3 => "small",
    x if x % 2 == 0 => "big and even",   // a guard: extra boolean condition
    _ => "big and odd",
}
```

A guarded arm only counts when its guard is true; otherwise matching continues with the next arm. The compiler cannot see through guards, so you still need a catch-all.

## Binding with @

`name @ pattern` tests a pattern and also keeps the value:

```rust
match age {
    n @ 0..=12 => format!("child, {n}"),
    n @ (13..=19) => format!("teen, {n}"),
    n => format!("adult, {n}"),
}
```

## Nested and tuple patterns

```rust
match (x, y) {
    (0, 0) => "origin",
    (0, _) | (_, 0) => "on an axis",
    (a, b) if a == b => "diagonal",
    _ => "elsewhere",
}

enum Shape { Rect { w: u32, h: u32 }, Circle(u32) }
match shape {
    Shape::Rect { w, h } if w == h => "square",
    Shape::Rect { .. } => "rectangle",
    Shape::Circle(r) => "circle",
}
```

## let-else: return early on a mismatch

`if let` nests the happy path inside a block. `let ... else` keeps the happy path flat and forces the else branch to leave (return, break, continue, panic):

```rust
let Some((cmd, arg)) = line.split_once(' ') else {
    return None;
};
```

## This lesson's exercise

A grader, a command parser, a point classifier and a shape describer.
