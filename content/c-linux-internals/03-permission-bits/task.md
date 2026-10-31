Implement `int may_access(const inode *n, const cred *c, int want)`; return 1 if every bit in `want` is allowed, else 0 (`want == 0` is allowed).

- Use `c->euid` and `c->egid` (and `groups`), never `c->uid` or `c->gid`.
- Pick exactly one class (owner, group, other) and test only its bits.
- Root (`euid == 0`) is allowed read and write on anything. Execute is allowed for root on a directory, or on a regular file with at least one of the three execute bits set.

All 512 modes are tried, for files and directories, with a range of credentials.
