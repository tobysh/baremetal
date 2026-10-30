**Challenge: merge two sorted lists.**

Write `Node *merge_sorted(Node *a, Node *b)`. Both lists are already in ascending order. Return one ascending list that contains every node of both.

- **Re-link the existing nodes.** Do not call `malloc` or `free`; the nodes in the result must be the very same nodes you were given.
- Equal values: take the node from `a` first.
- Either list may be empty (`NULL`). `a` and `b` must not be used by the caller afterwards.

The `Node` type is the one from the lesson. A tidy trick: start with a dummy `Node` on the stack and keep a `tail` pointer to the last node added.
