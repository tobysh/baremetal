Picture the arena as a row of blocks that always tile it with no gaps. `heap_alloc` turns part of one free block into a busy one; `heap_free` turns a busy block back into a free one and then makes sure no two free blocks sit side by side. Every function only ever touches one block and its two neighbours.
===
Order matters in `heap_alloc`: reject `n == 0` and `n > 256` **before** rounding up, or a huge `n` wraps to a tiny one. Then loop over `blocks[0..count-1]`, skipping busy blocks and blocks that are too short. If the chosen block is longer than `n`, a new free descriptor has to be inserted right after it.
===
Inserting at `i + 1` means shifting `blocks[i+1 .. count-1]` up by one, working from the end backwards so nothing is overwritten. Removing does the opposite. In `heap_free`, compare `p` with `arena + blocks[i].off` (an exact start), not with the block's range. After merging into the predecessor, the freed block's index is now `i - 1`: use that index when you check the successor.
===
The same insert-then-merge shape on a different list, a table of sorted, non-overlapping integer intervals:

```c
/* merge interval k with k+1 when they touch */
if (k + 1 < n && iv[k].end == iv[k + 1].start) {
    iv[k].end = iv[k + 1].end;
    for (unsigned j = k + 1; j + 1 < n; j++) iv[j] = iv[j + 1];
    n--;
}
```
