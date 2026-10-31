Start with `pt_fork` and `pt_wait`; they need only a scan over the table.
===
In `pt_exit`, reparent before you change the exiting process's own state, and do not touch free slots (their `ppid` is stale).
===
For `pt_descendants`, walk up from each process through `ppid` and see whether you meet `pid`. Cap the number of hops at `NPROC`.
