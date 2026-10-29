Write one helper that frees expired sessions: a session is dead if `now - last_seen > 300` or `now - created > 3600`. Call it at the start of every function.
===
Write `find(ss, token)` that returns the slot of a live session whose 16-character token is exactly equal (`strcmp` against a token you know is NUL-terminated is fine, a prefix does not match).
===
Format the token with `snprintf(buf, 17, "%016llx", (unsigned long long)rnd)`. In `ss_login`, handle `old_token` before checking for a collision, so a client who logs in again with their own old token and the same random value still fails the collision test only if the token is still live elsewhere.
