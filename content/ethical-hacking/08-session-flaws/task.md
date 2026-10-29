Implement the store (`SessionStore` is given):

- `void ss_init(SessionStore *ss)` clears it.
- `const char *ss_login(SessionStore *ss, const char *user, long now, uint64_t rnd, const char *old_token)` in this order: purge expired sessions; if `old_token` matches a live session, end it; compute the token (`%016llx` of `rnd`); if a live session already has this token, return `NULL`; if there is no free slot, return `NULL`; otherwise create the session (created = last_seen = `now`) and return a pointer to its token inside the store.
- `int ss_check(SessionStore *ss, const char *token, long now, char *user_out, size_t cap)` returns `1` if the token matches a live session (idle <= 300 s and age <= 3600 s), refreshing `last_seen` and copying the user name (truncated to `cap`, terminated; `cap` 0 writes nothing). Otherwise `0`, and an expired session is freed. `NULL` or short/long tokens never match.
- `void ss_logout(SessionStore *ss, const char *token)` ends that session on the server.

Long random sequences of logins, checks, logouts and time jumps are replayed against a reference.
