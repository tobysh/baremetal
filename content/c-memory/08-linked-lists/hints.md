`push_front` allocates one node with `malloc(sizeof *n)`, sets its `value`, sets its `next` to the old head, and returns the new node. The other functions walk the list with `cur = cur->next` until `cur` is `NULL`.
===
For `reverse_list` use three pointers: `prev` (starts `NULL`), `cur` (starts at `head`), and a temporary `next`. In each step: save `cur->next`, point `cur->next` back at `prev`, then advance `prev` and `cur`. For `free_list`, save `cur->next` before calling `free(cur)`.
===
```c
Node *prev = NULL;
Node *cur = head;
while (cur != NULL) {
    Node *next = cur->next;
    cur->next = prev;
    // advance prev and cur
}
return prev;
```
