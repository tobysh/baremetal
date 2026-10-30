Complete these methods on `List` (the structs are given). Keep the `Option<Box<Node>>` shape; do not store a `Vec` inside.

- `push_front(&mut self, value: i32)` adds at the head.
- `push_back(&mut self, value: i32)` adds at the tail.
- `pop_front(&mut self) -> Option<i32>` removes and returns the head value, or `None` if empty.
- `peek(&self) -> Option<&i32>` is a reference to the head value without removing it.
- `len(&self) -> usize` is the number of nodes.
- `to_vec(&self) -> Vec<i32>` lists the values from head to tail.
- `reverse(&mut self)` reverses the order in place.
- `remove(&mut self, value: i32) -> bool` removes the first node holding `value` and returns `true`, or returns `false` if there is none.
