Sequence: `pipe(p)`; fork the left child (`dup2(p[1], 1)`, close `p[0]` and `p[1]`, exec); fork the right child (`dup2(p[0], 0)`, `if (out_fd >= 0) dup2(out_fd, 1)`, close `p[0]`, `p[1]` and the original `out_fd`, exec).
===
In the parent, `close(p[0]); close(p[1]);` **before** waiting. Then `waitpid` for both pids. If the test hangs, a copy of `p[1]` is still open somewhere.
===
Convert the right child's status: `WIFEXITED` gives `WEXITSTATUS`, `WIFSIGNALED` gives `128 + WTERMSIG`. Retry `waitpid` on `EINTR`.
