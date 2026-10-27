Start with `let segs: Vec<&str> = path.split('/').collect();` and match on `segs.as_slice()`. Remember `/` becomes `["", ""]` and `/echo/hi` becomes `["", "echo", "hi"]`.
===
First decide whether the path is known (one `match` returning `bool`, using `if !text.is_empty()` as a guard for echo). If not known, return the 404. Then, if `method != "GET"`, return the 405. Only then build the real answer.
===
For `/add/a/b`, `match (a.parse::<i64>(), b.parse::<i64>())` with `(Ok(x), Ok(y))` for the sum and `_` for `(400, "bad number")`. Use `x.checked_add(y)` if you want to handle overflow without panicking.
