`count`, `total` and `in_stock` only read, so they take `&self` and use `iter()`. `count` is `find` (by name), then `map` to the quantity, then `unwrap_or(0)`. `total` maps each item to its `qty` and calls `sum()`. `in_stock` filters on `qty > 0`, maps to a clone of the name, and `collect()`s.
===
`add` and `remove` change an item, so they use `iter_mut().find(|i| i.name == name)`. In `add`, use `if let Some(item) = ... { item.qty += qty } else { self.items.push(Item { name: name.to_string(), qty }) }`.
===
For `remove`, match on the result of the `find`. A match arm with a guard, `Some(item) if item.qty >= qty => { item.qty -= qty; true }`, handles the success case, and `_ => false` covers both "no such item" and "not enough stock" without touching anything.
