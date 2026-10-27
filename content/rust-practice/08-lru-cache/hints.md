Write the private helper `touch(&mut self, key: &str)` from the lesson first. `get` and `put` both need it.
===
`get`: `let value = *self.map.get(key)?;` then `self.touch(key);` then `Some(value)`. Copying the `i32` out ends the borrow of `self.map`, so calling `self.touch` is allowed.
===
`put`: handle capacity 0 first and return `None`. If `self.map.contains_key(&key)`, insert the new value and `touch(&key)`; return `None`. Otherwise, if `self.map.len() == self.capacity`, evict.
===
To evict: `let oldest = self.order.remove(0);` and `self.map.remove(&oldest);`. Then insert the new value in the map, `self.order.push(key.clone())`, and return `Some(oldest)` when you evicted (use an `Option` variable that stays `None` otherwise).
