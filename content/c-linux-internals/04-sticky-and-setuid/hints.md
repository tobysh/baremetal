For `may_unlink`, the file's permission bits are not consulted at all. Which inode do you pass to `may_access`?
===
The sticky test is an extra condition applied after the normal directory check, never instead of it.
===
In `do_exec`, run `may_access(prog, c, P_X)` before you modify `c`, since the new identity must not influence the check.
