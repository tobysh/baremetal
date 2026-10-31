Draw the two tables for a sequence of calls on paper. Which table does `fd_advance` change, and which does `fd_open` change?
===
`fd_open` has two independent searches (a free descriptor, a free description). Do both searches before changing anything, so a failure leaves no half-made state.
===
In `fd_close`, decrement `refs` on the description the slot points to, then set the slot to `-1`. Reset the description only when `refs` reaches 0.
