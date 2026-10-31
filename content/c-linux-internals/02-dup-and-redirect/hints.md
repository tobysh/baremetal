Which number of tables does `dup` touch? Only the process table gets a new entry; the description just gains a reference.
===
In `fd_dup2`, `oldfd == newfd` has to be handled before the close, otherwise you close the descriptor you are about to copy.
===
Validate `oldfd` and `newfd` first. Then `fd_close(newfd)` if open, then copy the slot and `refs++`.
