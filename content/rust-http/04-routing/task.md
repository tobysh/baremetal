Write `fn route(method: &str, path: &str) -> (u16, String)`.

Known paths, for the `GET` method:

| path | result |
|---|---|
| `/` | `(200, "Welcome to Baremetal")` |
| `/health` | `(200, "ok")` |
| `/echo/<text>` | `(200, text)` |
| `/add/<a>/<b>` | `(200, sum)` where `a` and `b` parse as `i64` and the sum is written as a decimal string |

Rules:

- `<text>` is a single segment and must be non-empty. `/echo/` is **not** a known path. `/echo/a/b` is not either (too many segments).
- `/add/<a>/<b>` is a known path whatever `a` and `b` are. If either fails to parse as `i64` (or the sum overflows), the result is `(400, "bad number")`.
- Any `GET` to an unknown path returns `(404, "not found")`.
- Any method other than `GET` on a **known** path returns `(405, "method not allowed")`.
- Any method other than `GET` on an **unknown** path returns `(404, "not found")`.
- Methods are case-sensitive: `"get"` is not `"GET"`.
- Paths do not contain a query string or trailing slashes you need to normalise: `/health/` is just an unknown path.
