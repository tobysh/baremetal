---
title: Vectors and iterators
summary: Store lists in Vec, borrow them as slices, and process them with map, filter, sum and collect. Meet Option for missing values.
minutes: 18
difficulty: medium
id: rust-foundations/vectors-and-iterators
mode: harness
---
A C array has a fixed size, and you track its length yourself. Rust's `Vec<T>` is a growable list that owns its heap memory and knows its own length (and frees itself, per the last lesson).

```rust
let mut v = Vec::new();
v.push(10);
v.push(20);
let w = vec![1, 2, 3];   // macro shorthand
println!("{} {}", v.len(), w[0]);
```

Indexing out of range (`w[10]`) does not read garbage as in C. It **panics**: the program stops with a clear message. Safe, but still a crash, so prefer `w.get(10)`, which returns an `Option`.

## Slices

`&[T]` is a borrowed view of a run of elements: a pointer plus a length. Take `&[i32]` as a parameter and callers can pass a `&Vec<i32>`, an array, or part of one (`&v[1..3]`):

```rust
fn total(nums: &[i32]) -> i32 {
    let mut sum = 0;
    for n in nums {       // n is &i32
        sum += n;
    }
    sum
}
```

`for x in &v` borrows each element; `for x in v` would *move* the vector into the loop, and you couldn't use it afterwards. That is the common mistake.

## Iterator adapters

An **iterator** produces items one at a time. **Adapters** transform it, and nothing runs until a **consumer** pulls the results:

```rust
let squares: Vec<i32> = (1..=4).map(|x| x * x).collect(); // [1, 4, 9, 16]
let big: i32 = squares.iter().filter(|&&x| x > 4).sum();  // 25
```

- `|x| x * x` is a **closure**, a small unnamed function.
- `map` transforms each item; `filter` keeps items where the closure returns `true`.
- `sum` and `collect` consume the iterator. `collect` needs to know what to build, so annotate the type (`Vec<i32>`) or the function's return type.
- `.iter()` yields references (`&i32`), which is why closures sometimes need `&x` patterns or `*x`.

## Option: a value that might not exist

C uses `NULL` or `-1` for "nothing", and forgetting to check is a classic bug. Rust uses `Option<T>`: either `Some(value)` or `None`. You cannot use the inner value without handling `None`:

```rust
fn first(v: &[i32]) -> Option<i32> {
    if v.is_empty() { None } else { Some(v[0]) }
}

match first(&[7, 8]) {
    Some(n) => println!("first is {}", n),
    None => println!("empty"),
}
```

`v.len()` is a `usize`; to divide a float by it you must cast: `total / v.len() as f64`.
