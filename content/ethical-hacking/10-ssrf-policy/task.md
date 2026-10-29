Implement:

`int url_policy(const char *url, resolve_fn res, void *ctx, const char *const *allow, int nallow, uint32_t *pinned)` and

`int redirect_policy(const char *location, resolve_fn res, void *ctx, const char *const *allow, int nallow, uint32_t *pinned)`

`url_policy` returns `URL_OK` (0) or the **first** failing reason in this order:

1. no `://` in the text: `URL_MALFORMED` (2). Otherwise a scheme other than http/https (case-insensitive): `URL_SCHEME` (1).
2. The authority is the text up to the first `/`, `?`, `#` or end. If it is empty, has a backslash, has a character other than letters, digits, `.`, `-`, `:` and `@`, or if the host (the part after the last `@`, if any, up to a `:`) is empty, or a `:` is followed by nothing or by anything but digits: `URL_MALFORMED`.
3. An `@` in the authority: `URL_CREDENTIALS` (3).
4. A port that is not 80 (http) or 443 (https): `URL_PORT` (5). The default is fine.
5. The host (lowercase, one trailing `.` removed) not matching `allow[0..nallow-1]` (exact, or `*.suffix` meaning any longer host ending in `.suffix`): `URL_NOT_ALLOWED` (4).
6. Call `res(ctx, host, &ok)` **exactly once**, and only now. `ok == 0`: `URL_NO_ADDRESS` (7). Internal address (list in the lesson): `URL_INTERNAL` (6).
7. Otherwise store the address in `*pinned` (if not NULL) and return 0. On any failure leave `*pinned` alone.

`redirect_policy`: a location starting with `/` but not `//` returns 0 without resolving; one starting with `//` is `URL_MALFORMED`; anything else is `url_policy(location, ...)`.

Many generated URLs and addresses are tried.
