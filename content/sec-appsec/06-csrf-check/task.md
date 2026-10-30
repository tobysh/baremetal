Stdin: `SITE <origin>`, then `SESSION <sid> <token>` lines, a blank line, then requests as `method=M path=P sid=S origin=O token=T` fields (a field value of `-` means absent). Print `request N: allow` or `request N: deny (<reason>)` using the first rule that applies:

- method `GET`, `HEAD` or `OPTIONS`: `deny (state-change-over-get)` if the path contains `/delete` or `/transfer`, otherwise allow.
- otherwise (unsafe method): `deny (no-session)` if the sid is absent or unknown; `deny (cross-origin)` if an origin is present and is not exactly the site origin; `deny (bad-token)` if the token is absent or not exactly the session's token; otherwise allow.

Methods are case-sensitive. At most 100 sessions and 500 requests.
