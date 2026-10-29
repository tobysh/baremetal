The types `Node` and `DList` are given. Implement:

- `void dl_init(DList *l)` makes an empty list.
- `int dl_push_back(DList *l, int v)` and `int dl_push_front(DList *l, int v)` add a new node at that end. Return `0`, or `-1` if `malloc` fails.
- `int dl_insert_after(DList *l, Node *pos, int v)` adds a new node right after `pos` (which is a node of `l`, never `NULL`). Return `0` or `-1`.
- `void dl_remove(DList *l, Node *n)` unlinks node `n` (a node of `l`) and frees it.
- `Node *dl_find(const DList *l, int v)` returns the first node holding `v` from the front, or `NULL`.
- `void dl_reverse(DList *l)` reverses the list in place by relinking the existing nodes (the node addresses must not change).
- `void dl_free(DList *l)` frees every node and leaves an empty list.

Keep `len`, `head`, `tail` and every `prev`/`next` pointer consistent at all times.
