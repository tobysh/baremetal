`DNode` and `Deque` are defined in the starter. Implement:

- `void deque_init(Deque *d)` makes an empty deque (`head` and `tail` are `NULL`, `size` is `0`).
- `int push_front(Deque *d, int value)` and `int push_back(Deque *d, int value)` add a new node (use `malloc`) at that end. Return `0` on success, `-1` if `malloc` fails.
- `int pop_front(Deque *d, int *out)` and `int pop_back(Deque *d, int *out)` remove the node at that end, store its value in `*out`, free the node, and return `0`. If the deque is empty, return `-1` and leave `*out` alone.
- `int deque_remove(Deque *d, int value)` removes the **first** node (from the head) whose value equals `value`, frees it and returns `1`. If there is none, return `0`. It must work for the head, the tail, a middle node and the only node.
- `void deque_reverse(Deque *d)` reverses the order of the nodes in place, without allocating or freeing.
- `void deque_free(Deque *d)` frees every node and leaves the deque empty (as after `deque_init`).

Keep `size`, `head`, `tail` and every `prev`/`next` link correct after every call.
