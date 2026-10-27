`Stack` is mostly forwarding to `Vec`: `push`, `pop`, `last()` for `peek`, `len()` and `is_empty()`. `last()` already returns `Option<&T>`.
===
For `Queue::dequeue`: `if self.outbox.is_empty() { while let Some(x) = self.inbox.pop() { self.outbox.push(x); } }` and then `self.outbox.pop()`. Popping from one vector and pushing onto the other reverses the order.
===
`balanced`: for an opener, `push` it. For a closer, `pop` the stack and check it is the matching opener; if `pop` gives `None` or a different bracket, return `false`. Other characters do nothing. At the end the stack must be empty.
===
A `match c { '(' | '[' | '{' => ..., ')' => ..., ... }` keeps the three bracket kinds tidy. For closers you can compare with `stack.pop() != Some('(')` and return `false` on a mismatch.
