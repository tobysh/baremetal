Complete these in the starter. Both types must work for any `T`; no trait bounds are needed.

`Stack<T>`:
- `new() -> Stack<T>`
- `push(&mut self, item: T)`
- `pop(&mut self) -> Option<T>` removes and returns the top, or `None` if empty
- `peek(&self) -> Option<&T>` looks at the top without removing it
- `len(&self) -> usize` and `is_empty(&self) -> bool`

`Queue<T>` (two vectors, `inbox` and `outbox`, as the lesson describes):
- `new() -> Queue<T>`
- `enqueue(&mut self, item: T)`
- `dequeue(&mut self) -> Option<T>` removes the oldest item, or `None` if empty. Items must come out in the order they went in, even when enqueues and dequeues are mixed.
- `len(&self) -> usize` and `is_empty(&self) -> bool` (count both vectors)

`fn balanced(s: &str) -> bool` uses a `Stack<char>`. The brackets are `()`, `[]` and `{}`; all other characters are ignored. It returns `true` when every closer matches the most recent unclosed opener and nothing is left open. The empty string is balanced; `"(]"`, `"(()"` and `")"` are not.
