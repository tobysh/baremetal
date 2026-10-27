Write small free functions that take the slot: `fn insert_into(slot: &mut Option<Box<Node>>, value: i32) -> bool`. Then `Tree::insert` is just `insert_into(&mut self.root, value)`.
===
`contains`: `match slot { None => false, Some(n) => if value == n.value { true } else if value < n.value { search(&n.left, value) } else { search(&n.right, value) } }`. A `while let` loop with a cursor works too.
===
`in_order`: write `fn walk(slot: &Option<Box<Node>>, out: &mut Vec<i32>)` that, for `Some(n)`, calls `walk(&n.left, out)`, then `out.push(n.value)`, then `walk(&n.right, out)`.
===
`height` of a slot is `0` for `None`, and `1 + max(height(left), height(right))` for a node. For `min`, loop: `let mut cur = self.root.as_ref()?; while let Some(l) = cur.left.as_ref() { cur = l; } Some(cur.value)`.
