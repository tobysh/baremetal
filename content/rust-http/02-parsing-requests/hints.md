Start with `raw.split_once("\r\n\r\n")`. Use `unwrap_or((raw, ""))` for the no-separator case. Then `head.split("\r\n")` gives you an iterator whose first item is the request line.
===
Get the request line with `lines.next()`. An empty `head` means there is no request line: check `head.is_empty()` first. Call `parse_request_line(first)` and turn `None` into an `Err` with `.ok_or_else(|| ...)?`.
===
For each remaining line, `let (name, value) = line.split_once(':').ok_or_else(|| format!("malformed header: {}", line))?;` and then `headers.insert(name.trim().to_lowercase(), value.trim().to_string());`.
