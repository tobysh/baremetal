Implement (using the given `may_access`):

- `int may_unlink(const inode *dir, const inode *file, const cred *c)`: 0 if `dir` is not a directory, or if `c` lacks write and search permission on `dir`. If `dir` has the sticky bit, also require `c->euid` to be 0, the file's owner or the directory's owner. Otherwise return 1.
- `int do_exec(const inode *prog, cred *c)`: return `-1` without changing `c` if `prog` is a directory or `c` may not execute it. Otherwise set `c->euid = prog->uid` when setuid is set and `c->egid = prog->gid` when setgid is set (never touch `uid`, `gid` or the supplementary groups), and return 0.
