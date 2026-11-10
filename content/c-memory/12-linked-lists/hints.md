Start with `typedef struct Node { int value; struct Node *next; } Node;` and a `main` that reads one command word with `scanf("%15s", cmd)`. Compare it with `strcmp` (from `<string.h>`); for `push`, read the number next.
===
Write `push_front(Node *head, int value)` first: allocate, set `value`, point `next` at the old head, return the new node. Check it with `push` and `print` before adding more commands. The caller does `head = push_front(head, n);`.
===
`pop` needs the old head's value and next pointer before you `free` it. Save them, free the node, then return the next pointer as the new head.
===
Reverse by walking once: save `cur->next`, point `cur->next` at the previous node, move both forward. When you finish, the last node you visited is the new head. Free every node before `main` returns.
