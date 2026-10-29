Set `bad |= (p==0) | (p>bs)` first, then loop over every index `i` in the final block.
===
For each `i`, use `in = i < p` and `bad |= in & (blk[n-1-i] != p)`. Count `work` inside the loop and decide only after it finishes.
