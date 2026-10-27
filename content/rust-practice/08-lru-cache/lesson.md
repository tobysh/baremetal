---
title: A Least-Recently-Used Cache
summary: Combine a HashMap with a Vec to build an LRU cache, and learn to keep two data structures in step without fighting the borrow checker.
minutes: 30
difficulty: hard
id: rust-practice/lru-cache
mode: harness
---
A **cache** keeps answers you have already worked out so you do not have to work them out again. But memory is finite, so a cache has a capacity, and when it is full something must be forgotten. An **LRU** (least recently used) cache forgets the item that has gone longest without being touched. It is a bet that what you used lately you will use again soon, and it is a good one.

## Two structures, one truth

A `HashMap` finds a value by key quickly, but it has no idea about *order*. A `Vec` can remember order, but searching it is slow. Use both:

```rust
use std::collections::HashMap;

struct LruCache {
    capacity: usize,
    map: HashMap<String, i32>,
    order: Vec<String>, // oldest first, newest last
}
```

The golden rule is that the two must always describe the same set of keys. Every method that changes one must change the other. Writing a small private helper for the shared step ("move this key to the newest position") keeps the rule in one place:

```rust
fn touch(&mut self, key: &str) {
    if let Some(pos) = self.order.iter().position(|k| k == key) {
        let k = self.order.remove(pos);
        self.order.push(k);
    }
}
```

`position` finds where the key sits, `remove` takes it out and returns it, `push` puts it at the newest end. (This is O(n) on the length of the list. Real caches use a linked list to make it O(1); for a lesson, clarity wins.)

## Borrowing and &str keys

`HashMap<String, i32>::get` accepts a `&str`, even though the keys are `String`s, because `String: Borrow<str>`. So a method can take `key: &str` for lookups and only allocate a `String` when it needs to store one.

When a method needs both to read from the map and then change the order, finish one before starting the other. A value copied out of the map (`i32` is `Copy`) no longer borrows the map, so you can then modify `self.order` freely.

```rust
fn get(&mut self, key: &str) -> Option<i32> {
    let value = *self.map.get(key)?; // the borrow of the map ends here
    self.touch(key);
    Some(value)
}
```

## This lesson's exercise

Implement `get`, `put`, `contains`, `len` and `keys_oldest_first`. Reading with `get` counts as use, so it refreshes the key; `contains` is a peek and does not.
