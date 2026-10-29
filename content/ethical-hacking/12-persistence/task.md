Read stdin (at most 500 lines per section; fields have no spaces; ignore blank lines). Entries are identified by `KIND NAME`. For each current entry, in input order:

- not in the baseline: `NEW KIND NAME flags=F level=L`
- in the baseline with a different PATH and/or USER: `CHANGED KIND NAME changed=C flags=F level=L`, where `C` is `path`, `user` or `path,user`
- identical: print nothing.

Then, for each baseline entry missing from current, in baseline order: `REMOVED KIND NAME`.

`F` lists comma-separated, in this order, the flags of the **current** path/user: `temp-path` (path starts with `/tmp/`, `/var/tmp/` or `/dev/shm/`) or `home-path` (starts with `/home/`), `hidden` (last path component starts with `.`), `root` (user is `root`); `-` if none. Points: temp-path 3, home-path 2, hidden 2, root 1. `L` is `high` for 4 or more, `medium` for 2 or 3, else `low`. If nothing was printed, print `no persistence changes`.

Other snapshots are tried, including changes that touch only the user.
