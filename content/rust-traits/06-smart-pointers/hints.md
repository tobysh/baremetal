`new_node`: `Rc::new(RefCell::new(Node { value, children: Vec::new() }))`. `owners` is `Rc::strong_count(node)`.
===
`add_child`: `parent.borrow_mut().children.push(Rc::clone(child));`. Cloning an `Rc` copies the pointer and bumps the count; both handles see the same node.
===
`sum` is recursive: start with `let n = node.borrow();`, then `let mut total = n.value;` and add `sum(c)` for every `c` in `&n.children`.
===
`double_all`: `let mut n = node.borrow_mut(); n.value *= 2;` then loop `for c in &n.children { double_all(c); }`. The children are different `RefCell`s, so borrowing them while the parent is borrowed is fine.
