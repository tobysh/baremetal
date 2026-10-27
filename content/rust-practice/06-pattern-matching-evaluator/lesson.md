---
title: Patterns in Depth: An Expression Evaluator
summary: Use match guards, bindings, nested enum patterns and slice patterns to evaluate and simplify arithmetic expression trees.
minutes: 35
difficulty: hard
id: rust-practice/pattern-matching-evaluator
mode: harness
---
`match` is far more than a switch. A **pattern** can take apart nested data, bind pieces to names, test conditions and check lengths, all in one line, and the compiler checks you covered every case. This lesson gathers the tools, then uses them on a small calculator.

## The toolbox

```rust
match n {
    0 => "zero",
    1 | 2 | 3 => "small",          // alternatives
    4..=9 => "medium",             // ranges
    x if x < 0 => "negative",      // guard: an extra condition
    big @ 10..=99 => { /* big is bound to the value */ "big" }
    _ => "huge",
}
```

A **guard** (`if ...`) runs only after the pattern matches. A binding with `@` keeps the whole value while also testing it against a pattern.

**Nested patterns** reach through enums and tuples:

```rust
match (left, right) {
    (Some(a), Some(b)) => a + b,
    (Some(a), None) | (None, Some(a)) => a,  // same name in each alternative
    (None, None) => 0,
}
```

**Slice patterns** match on the length and shape of a slice:

```rust
match items {
    [] => "empty",
    [only] => "one",
    [first, .., last] if first == last => "bookends",
    [first, rest @ ..] => "a first item and some more",
}
```

`..` skips any number of items, and `rest @ ..` captures them as a smaller slice.

## Expression trees

A calculator's input, once parsed, is a tree. Each node is a number or an operation holding smaller expressions. Because an `Expr` contains `Expr`s, the children sit in a `Box`:

```rust
enum Expr {
    Num(i64),
    Neg(Box<Expr>),
    Add(Box<Expr>, Box<Expr>),
    // ...
}
```

To **evaluate** it, match on the node, evaluate the children, combine. To **simplify** it, match on the *shape*: `x * 1` becomes `x`, `x + 0` becomes `x`, and `--x` becomes `x`. Matching on a pair of already-simplified children, `(Num(0), x) | (x, Num(0))`, expresses "zero on either side" in a single arm. Since the values are moved out of the boxes with `*boxed`, you consume the old tree and build a new one.

## This lesson's exercise

Write `eval` (with checked arithmetic that reports errors rather than panicking), `simplify`, and a slice-pattern function `describe`. This is the longest lesson so far: take it one function at a time.
