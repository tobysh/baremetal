Complete these methods on `Tree` (the structs are given). Values are `i32`.

- `insert(&mut self, value: i32) -> bool` adds the value and returns `true`; if it is already present, change nothing and return `false`.
- `contains(&self, value: i32) -> bool`
- `in_order(&self) -> Vec<i32>` returns all values in ascending order. An empty tree gives an empty vector.
- `height(&self) -> usize` is the number of nodes on the longest path from the root down. An empty tree has height `0`, a single node has height `1`.
- `min(&self) -> Option<i32>` is the smallest value, or `None` for an empty tree. Follow the left children; do not collect everything first.

Keep the shape `Option<Box<Node>>`; do not store values in a `Vec` and sort them.
