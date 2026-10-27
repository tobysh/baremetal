Draw a three-node list and write down which pointers change for each operation. Always handle the "list was empty" and "list becomes empty" cases separately: `head` and `tail` both change.
===
For `pop_front`: `DNode *n = d->head;` then `d->head = n->next;`. If `d->head` is now `NULL`, set `d->tail = NULL`, otherwise set `d->head->prev = NULL`. Then read the value, `free(n)` and decrement `size`. `pop_back` is the mirror image.
===
`deque_remove`: walk with `for (DNode *n = d->head; n != NULL; n = n->next)`. When you find the value, fix the link before it (`n->prev->next = n->next`, or `d->head = n->next` if there is no `prev`) and the link after it (`n->next->prev = n->prev`, or `d->tail = n->prev` if there is no `next`). Then free `n`.
===
`deque_reverse`: loop with `DNode *n = d->head; while (n) { DNode *next = n->next; n->next = n->prev; n->prev = next; n = next; }` then swap `d->head` and `d->tail`.
