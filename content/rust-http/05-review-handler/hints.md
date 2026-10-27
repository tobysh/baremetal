Start with `body_matches_length`. `req.headers.get("content-length")` gives an `Option<&String>`. `None` means there is nothing to check, so return `true`. For `Some(v)`, `v.parse::<usize>()` is a `Result`: on `Ok(n)` compare `n == req.body.len()`, and on `Err(_)` return `false`.
===
In `handle`, `match parse_request(raw)` with `Ok(r) => r` and `Err(_) => return build_response(400, "text/plain", "bad request")`. Then do the same early return when `!body_matches_length(&req)`.
===
After those checks: `if req.method == "POST" && req.path == "/shout" { return build_response(200, "text/plain", &req.body.to_uppercase()); }`. Finally `let (status, body) = route(&req.method, &req.path);` and `build_response(status, "text/plain", &body)`.
