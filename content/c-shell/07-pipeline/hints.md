Allocate `pid_t pids[n]` (a variable-length array is fine) and `int prev = in_fd;`. For `i < n - 1` call `pipe(p)`; for the last command there is no pipe.
===
In every child close `prev` (if it is not -1 and was dup'ed), both pipe ends, `in_fd` and `out_fd` after your `dup2` calls. Careful: do not close a descriptor you still need for a later `dup2`, and do not close fd 0 or 1 by accident when `in_fd` or `out_fd` equals them.
===
In the parent: `if (i > 0) close(prev);` (only close pipe ends you created, never the caller's `in_fd`), `close(p[1])`, `prev = p[0]`. After the loop, wait for all pids; the return value comes from `pids[n - 1]`.
