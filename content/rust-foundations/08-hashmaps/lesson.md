---
title: HashMaps
summary: Store key-value pairs in a HashMap and use the entry API to count and group things without checking twice.
minutes: 15
difficulty: medium
id: rust-foundations/hashmaps
mode: harness
---
C has no built-in dictionary; you would write your own hash table with arrays, a hash function and linked lists. Rust's standard library includes one: `HashMap<K, V>` maps keys to values with fast average lookup. It lives in a module, so import it:

```rust
use std::collections::HashMap;

let mut ages: HashMap<String, u32> = HashMap::new();
ages.insert(String::from("Ada"), 36);
ages.insert(String::from("Alan"), 41);
```

`insert` replaces the value if the key already exists. The map **owns** its keys and values, so inserting a `String` moves it in (the ownership rule from earlier).

## Looking things up

`get` returns an `Option`, because the key may be missing, and it gives a reference to the value:

```rust
match ages.get("Ada") {
    Some(age) => println!("Ada is {}", age),
    None => println!("unknown"),
}
```

(`get` accepts a `&str` here even though the keys are `String`s.) Other handy methods: `contains_key`, `remove`, `len`. Iterating with `for (key, value) in &ages` visits entries in **no particular order**, so never rely on the order, and sort the keys if you need a stable one.

## Counting: the entry API

A very common job: increase a counter for a key, creating it if it is new. The naive way looks up the key twice:

```rust
if counts.contains_key(&word) {
    *counts.get_mut(&word).unwrap() += 1;
} else {
    counts.insert(word, 1);
}
```

The **entry API** does it in one step:

```rust
*counts.entry(word).or_insert(0) += 1;
```

`entry(word)` finds the slot for that key. `or_insert(0)` puts in `0` if the slot was empty, and either way returns a **mutable reference** to the value. The leading `*` (dereference) follows the reference so you can add to the number itself, just like `*ptr += 1` in C. Forgetting the `*` is the most common mistake here; the compiler will tell you that you can't add to a `&mut usize`.

## Cleaning text

For the exercise you will break text into words. Some handy `&str` methods:

- `text.split_whitespace()` yields each word, skipping any run of spaces or newlines.
- `word.to_lowercase()` returns a new `String`.
- `word.trim_matches(|c: char| c.is_ascii_punctuation())` removes punctuation from both ends only, so `"don't"` keeps its apostrophe but `"hello,"` loses its comma.
- `to_string()` or `String::from` turns a `&str` into an owned `String` for use as a key.

Join these together in a loop and the `entry` line above, and the whole function is about six lines long.
