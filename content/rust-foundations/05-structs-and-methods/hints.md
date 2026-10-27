A `Stack` just holds a `Vec<i32>` in `items`. `new` should build a `Stack` with an empty vector: `Stack { items: Vec::new() }`.
===
`self.items` is the vector. `push` and `pop` map directly to `Vec::push` and `Vec::pop` (and `pop` already returns an `Option<i32>`). `peek` can use `self.items.last()`, which gives `Option<&i32>`.
===
For `peek`, copy the value out of the reference: `self.items.last().copied()`. `len` is `self.items.len()`, and `is_empty` is `self.items.is_empty()`.
