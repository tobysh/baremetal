---
title: Deriving Ord, Hash and Default
summary: Let derive write PartialEq, Ord, Hash, Clone and Default for you, understand field-order comparison and use struct update syntax.
minutes: 20
difficulty: easy
tags: derive, traits, ordering, default
id: rust-bridge/derive-ord-default
mode: harness
---
Many traits have an obvious implementation: "do it to every field". `#[derive(...)]` writes it for you.

```rust
#[derive(Debug, Clone, PartialEq, Eq, Hash, PartialOrd, Ord, Default)]
struct Version { major: u32, minor: u32, patch: u32 }
```

What each one gives you:

- `Debug`: `{:?}` printing. `Clone`: `.clone()`. `Default`: `Version::default()` with every field defaulted (`0`, `""`, `false`, `None`, empty `Vec`).
- `PartialEq`, `Eq`: `==` and `!=`. `Hash`: usable as a `HashMap` key or in a `HashSet` (needs `Eq`).
- `PartialOrd`, `Ord`: `<`, `>`, `max`, `sort()`, and use as a `BTreeMap` key.

## How derived ordering works

Structs compare **field by field in declaration order**, like words in a dictionary. So the field order is part of your design:

```rust
Version { major: 1, minor: 9, patch: 0 } < Version { major: 2, minor: 0, patch: 0 }  // true
```

**Enums** compare by variant order first (earlier variants are smaller), then by their contents:

```rust
#[derive(PartialEq, Eq, PartialOrd, Ord)]
enum Rank { Low, Mid, High }          // Low < Mid < High
```

Floats are only `PartialOrd` (NaN breaks total order), so a struct with an `f64` cannot derive `Ord` or `Hash`.

## Struct update syntax

`..` fills the remaining fields from another value, typically the default:

```rust
let c = Config { verbose: true, ..Config::default() };
```

## This lesson's exercise

A version type, a card type and a settings type, all leaning on derive.
