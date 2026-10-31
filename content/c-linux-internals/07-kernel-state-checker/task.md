Implement the three functions in the starter, using the given library:

- `ks_open(k, pid, n, want)`: `-1` unless `pid` is a valid running process and `may_access(n, &k->cr[pid], want)`; then `fd_open` on that process's table with `n->ino`.
- `ks_exit(k, pid, status)`: call `pt_exit` first; if it refuses, return `-1` and change nothing. Then close every descriptor of `pid`, set its pending set to 0, and `sig_send` `SG_CHLD` to its parent. Return 0.
- `ks_check(k)`: return the OR of the `V_*` bits described in the lesson. Count references over **all** pids, any state, but report `V_DANGLING` only for running processes and `V_DEAD_FDS` only for non-running ones.

Pids run from 1 to `NPROC - 1`; index 0 is unused and ignored.
