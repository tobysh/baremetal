Stdin: first line `W cap`, then at most 1000 messages `<msg-time> <receive-time> <nonce>` in non-decreasing receive time (nonce at most 31 characters). For each print `N: <result>`:

1. `stale` if `msg-time < receive-time - W`; `future` if `msg-time > receive-time + W` (neither is stored).
2. Otherwise first forget every remembered nonce whose message time is `< receive-time - W`.
3. `replay` if the nonce is still remembered.
4. `cache-full` if `cap` nonces are remembered (nothing is stored).
5. Otherwise `accept` and remember the nonce with its message time.

Tests use many window sizes and boundary values.
