Implement the four functions described in the starter comments. Details the tests check:

- `pt_fork` takes the lowest free pid from 2 upward and fails if the parent is not `P_RUN`.
- `pt_exit` keeps the slot, sets `P_ZOMBIE` and the status, and moves **every** child (running or zombie) to parent 1. It fails for pid 1, invalid pids and non-running processes, without changing anything.
- `pt_wait` reaps the lowest-numbered zombie child, writes the status (if the pointer is not `NULL`), clears the whole slot, and returns the pid. Otherwise it returns 0 or -1 as the starter explains.
- `pt_descendants` counts every existing process below `pid`, zombies included, at any depth. Use a loop with a step bound rather than trusting the tree to be well formed.
