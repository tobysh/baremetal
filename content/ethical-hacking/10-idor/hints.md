Look the invoice up by id first. If it does not exist, return 404. Then compare owner to `who.uid` (admins always pass). Both failures give the same 404.
===
For the log, keep the timestamps of this user's 404 records and, at each new 404, count how many fall in `(t - 60, t]`. Other users' records and other statuses are ignored.
