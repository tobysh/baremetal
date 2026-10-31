Implement:

- `fd_dup(s, t, fd)`: pick the lowest free number, point it at `fd`'s description, increment `refs`. Return the new number, or `-1` for a bad `fd` or a full table.
- `fd_dup2(s, t, oldfd, newfd)`: `-1` if `oldfd` is not open or `newfd` is outside `0..NFD-1`. If they are equal return `newfd` and change nothing. Otherwise close `newfd` if open (use `fd_close`, so the reference count is handled), then make it share `oldfd`'s description. Return `newfd`.

Check the arguments **before** closing anything: a failed call must leave all state unchanged.
