The `Node` type is already defined in the starter code. Write four functions:

- `Node *push_front(Node *head, int value)` allocates a new node holding `value`, links it in front of `head`, and returns the new head. `head` may be `NULL` (empty list). If allocation fails, return `head` unchanged.
- `size_t list_length(const Node *head)` returns the number of nodes. An empty list has length 0.
- `Node *reverse_list(Node *head)` reverses the list by relinking the existing nodes (no new allocation) and returns the new head. Reversing an empty list or a one-node list is fine.
- `void free_list(Node *head)` frees every node. `head` may be `NULL`.
