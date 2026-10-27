The starter defines `type NodeRef = Rc<RefCell<Node>>` and `struct Node { value: i32, children: Vec<NodeRef> }`. Implement:

- `fn new_node(value: i32) -> NodeRef` creates a node with no children.
- `fn add_child(parent: &NodeRef, child: &NodeRef)` adds a *shared handle* to `child` to the parent's children (use `Rc::clone`, do not copy the node's data).
- `fn sum(node: &NodeRef) -> i32` returns the value of the node plus all its descendants.
- `fn double_all(node: &NodeRef)` doubles the value of the node and every descendant, in place. Changes must be visible through every handle.
- `fn owners(node: &NodeRef) -> usize` returns the number of `Rc` owners of the node.
