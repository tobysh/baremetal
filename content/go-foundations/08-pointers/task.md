Implement these functions:

- `Swap(a, b *int)` swaps the two integers the pointers point to.
- `Increment(p *int)` adds 1 to the integer `p` points to, and does nothing if `p` is `nil`.
- `Push(head *Node, v int) *Node` returns a new list whose first element is `v` followed by the list `head` (which may be `nil`).
- `Reverse(head *Node) *Node` reverses the list in place by relinking the nodes (don't allocate new ones) and returns the new head.
