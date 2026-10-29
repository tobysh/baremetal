Read stdin (at most 1000 lines, fields without spaces, malformed lines ignored). For each `S` and `C` line, in input order, whose `runs_as` is not `guest` and whose exec path is listed as an `F`/`D` entry:

- if the exec file is writable by guest: `writable-exec LABEL path=P fix=FIX`
- otherwise, if its parent directory (the text before the last `/`, or `/` if empty) is listed, writable by guest and **not sticky** (`01000` clear): `writable-dir LABEL dir=D fix=FIX`

`LABEL` is the service name, or `cron` for `C` lines. Then, for each `F` line in input order whose mode has the setuid bit `04000`, owner `root`, and which is writable by guest: `setuid-writable path=P fix=FIX`.

`FIX` for the entry in question: if guest is its owner and the owner-write bit is set, `chown root`; otherwise `chmod MODE` where `MODE` is the entry's mode with `0002` cleared and, if its group is one of guest's groups, `0020` cleared too, printed as four octal digits (`%04o`). If nothing is printed, print `no privilege escalation paths found`.

Other listings are tried, including sticky directories and entries missing from the listing.
