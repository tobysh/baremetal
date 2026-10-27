`line.split(' ').collect::<Vec<&str>>()` gives you the pieces. Check `parts.len()` before you index into it.
===
Return `None` early if `parts.len() != 3`, and again if `!parts[2].starts_with("HTTP/")`. A doubled space produces an empty piece, so it fails the length check on its own.
===
After the checks, build the struct with `parts[0].to_string()`, `parts[1].to_string()` and `parts[2].to_string()` and wrap it in `Some(...)`.
