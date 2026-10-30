`push_front`: `let old = self.head.take(); self.head = Some(Box::new(Node { value, next: old }));`. `pop_front`: `self.head.take().map(|node| { self.head = node.next; node.value })`. `peek`: `self.head.as_ref().map(|n| &n.value)`.
===
`len` and `to_vec`: `let mut cur = &self.head; while let Some(node) = cur { ...; cur = &node.next; }`. For `push_back`, walk a `&mut` cursor: `let mut cur = &mut self.head; while cur.is_some() { cur = &mut cur.as_mut().unwrap().next; }` then `*cur = Some(Box::new(...))`.
===
`reverse`: keep `prev = None` and `cur = self.head.take()`. Loop `while let Some(mut node) = cur { cur = node.next.take(); node.next = prev; prev = Some(node); }`, then `self.head = prev`.
For `remove`, move the `&mut` cursor while the link is `Some` and its value differs: `while cur.as_ref().map_or(false, |n| n.value != value) { cur = &mut cur.as_mut().unwrap().next; }`. Then `match cur.take() { Some(node) => { *cur = node.next; true } None => false }`.
