The starter defines `struct Stack { items: Vec<i32> }`. Implement these in an `impl Stack` block:

- `fn new() -> Stack` creates an empty stack.
- `fn push(&mut self, x: i32)` adds `x` on top.
- `fn pop(&mut self) -> Option<i32>` removes and returns the top item, or `None` if empty.
- `fn peek(&self) -> Option<i32>` returns the top item without removing it, or `None` if empty.
- `fn len(&self) -> usize` returns the number of items.
- `fn is_empty(&self) -> bool` returns `true` if there are no items.
