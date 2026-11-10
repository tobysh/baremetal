Read standard input: at most 1000 lines, each a syslog-style line. Handle at most 256 source addresses and 256 distinct usernames per address; names and addresses are at most 63 characters.

Only lines containing `sshd[` are considered. Within them:

- ` Failed password for [invalid user ]USER from IP ...` is a failure for `IP` by `USER`. The text `invalid user ` is skipped when present; a user named `invalid` with no `user ` after it is an ordinary user.
- ` Accepted password for USER from IP ...` is an acceptance.
- Everything else (public-key lines, connection notices, other programs, even if they contain the same words) is ignored.

For every address with at least 5 failures print `IP failures=N users=U`, where `N` is its total failures in the whole input and `U` the number of distinct usernames it failed as. If the address had an acceptance after its fifth failure (counting lines in input order), append ` ACCEPTED as USER` for the first such acceptance. Acceptances before the fifth failure are ignored.

Rows are sorted by failures descending, then by `strcmp` of the address. If no address qualifies print `no brute force detected`. One newline per row, nothing else.

The hidden tests use different logs: counts of exactly four and five, acceptances before and after the threshold, ties in the failure count, a user named `invalid`, non-`sshd` lines carrying the same wording and logs without a final newline. Keep the program reading stdin and writing to stdout.
