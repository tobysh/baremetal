Write `dl_insert_after` first as the general case, then notice that `push_back` is "insert after the tail" (when the list is not empty) and `push_front` needs its own head handling.
===
Allocate the node with a small helper `new_node(v)` that sets `prev` and `next` to `NULL`. Then wire it in. For `push_front` into an empty list, both `head` and `tail` become the new node.
===
`dl_remove`: `if (n->prev) n->prev->next = n->next; else l->head = n->next;` and the mirror line for `tail`. Then `free(n); l->len--;`. For `dl_reverse`, walk the list once and swap each node's `prev` and `next`; afterwards swap `l->head` and `l->tail`. Save `next` before swapping or you walk the wrong way.
