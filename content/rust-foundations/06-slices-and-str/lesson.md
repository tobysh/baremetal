---
title: Slices and &str vs String
summary: Borrow part of an array, Vec or string with slices, and learn why functions should take &str and &[T] instead of &String and &Vec<T>.
minutes: 18
difficulty: medium
tags: slices, strings
id: rust-foundations/slices-and-str
mode: harness
---
In C, "a pointer plus a length" is something you pass around as two arguments and keep in sync by hand. Rust has a built-in type for it: the **slice**. It is a borrowed view of a run of elements that knows its own length and is bounds-checked.

## Slices of arrays and vectors

`&[T]` is a slice of `T`. Make one with a range:

```rust
let v = vec![10, 20, 30, 40, 50];
let mid: &[i32] = &v[1..4];   // [20, 30, 40]
let head = &v[..2];           // [10, 20]
let tail = &v[3..];           // [40, 50]
let all = &v[..];             // everything
```

`a..b` includes `a` and excludes `b`. A slice does not own anything and copies nothing: it is a pointer and a length. Asking for a range outside the data panics, so check with `len()` or use `v.get(1..4)`, which returns an `Option`.

## String and &str

The same split exists for text:

| | `String` | `&str` |
|---|---|---|
| owns its bytes | yes (heap) | no, a view |
| growable | yes | no |
| typical use | building and storing text | reading text, parameters |

A string literal like `"hello"` is a `&'static str` that lives in the program itself. `&str` is to `String` what `&[T]` is to `Vec<T>`.

```rust
let owned = String::from("hello world");
let hello: &str = &owned[0..5];   // "hello"
```

String slice ranges are **byte** positions and must fall on character boundaries (see the previous lesson), or the program panics.

## Take the most general parameter

Because a `&String` automatically converts to a `&str`, and a `&Vec<T>` to a `&[T]`, the slice type accepts more callers:

```rust
fn shout(s: &str) -> String { s.to_uppercase() }

shout("literal");
shout(&String::from("owned"));
shout(&owned[0..5]);
```

```rust
fn total(nums: &[i32]) -> i32 { nums.iter().sum() }

total(&[1, 2, 3]);       // array
total(&v);               // Vec
total(&v[1..3]);         // part of a Vec
```

So write `&str` and `&[T]` in parameters unless you need the owned type. In the previous lesson you wrote `&Vec<String>` to practise; real code would write `&[String]`.

## Returning slices

A function may return a slice of its input. The result borrows from the input, so the input cannot be changed or dropped while the slice is in use:

```rust
fn first_two(v: &[i32]) -> &[i32] {
    if v.len() < 2 { v } else { &v[..2] }
}
```

## Handy methods

- `s.split_whitespace()` iterates the words, skipping any amount of spaces.
- `s.find(c)` gives the byte position of a char as `Option<usize>`.
- `s.split_once(c)` returns `Option<(&str, &str)>` for text before and after the first `c`.
- `slice.first()`, `slice.last()` return `Option<&T>`; `slice.join("-")` joins strings.
- `c.len_utf8()` is how many bytes a `char` takes.

## This lesson's exercise

Four functions that take slices and return slices (or a joined `String`). Nothing should be copied unless the return type is `String`.
