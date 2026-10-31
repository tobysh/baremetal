**Challenge: dup3 and closefrom.**

Using the same tables, add two functions:

- `int fd_dup3(sysfiles *s, fdtable *t, int oldfd, int newfd, int flag)` behaves like `fd_dup2` with two differences that real Linux has: if `oldfd == newfd` it fails with `-1` (no no-op), and `flag` is stored in a new field you add to **no structure**: instead return `newfd | (flag ? 0x100 : 0)` on success so the caller can see what was requested. (The model has no close-on-exec flag; this is a stand-in for it.)
- `int fd_closefrom(sysfiles *s, fdtable *t, int lo)` closes every open descriptor `>= lo` (a negative `lo` means 0) and returns how many were closed.

Both must leave reference counts correct when descriptors share a description. A failed `fd_dup3` changes nothing.
