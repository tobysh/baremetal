Keep a `static Block *head;`. In `heap_init`: `uintptr_t s = (uintptr_t)start; uintptr_t aligned = (s + 15) & ~(uintptr_t)15;` then subtract the difference from `size` and round `size` down with `& ~(size_t)15`. The first block lives at `aligned` with `size = total - sizeof(Block)`.
===
To split a block `b` that must give `want` bytes: `Block *rest = (Block *)((uint8_t *)(b + 1) + want);` then `rest->size = b->size - want - sizeof(Block); rest->free = 1; rest->next = b->next; b->next = rest; b->size = want;`.
===
Coalescing: loop `for (Block *b = head; b && b->next; )`. If both `b` and `b->next` are free, do `b->size += sizeof(Block) + b->next->size; b->next = b->next->next;` and **do not advance**, since the new neighbour may be free too. Otherwise advance `b = b->next`.
===
`kcalloc`: `if (size && count > (size_t)-1 / size) return NULL;` then `kmalloc(count * size)` and a loop that zeroes every byte (there is no `memset` to rely on).
===
`heap_free_bytes` is a loop adding `b->size` for each block with `b->free`.
