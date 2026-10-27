Loop on `stream.read(&mut chunk)`. A result of `Ok(0)` means the client closed, so stop. After each read, append `&chunk[..n]` to a `Vec<u8>` and stop once it contains `\r\n\r\n`.
===
`buf.windows(4).any(|w| w == b"\r\n\r\n")` tests for the blank line, even when it is split across two reads. Also stop when `buf.len() >= 8192`, and treat `Err(_)` from `read` like EOF.
===
After the loop: `let text = String::from_utf8_lossy(&buf);`, then `text.lines().next().unwrap_or("")` is the request line. `match parse_request_line(first)`: on `Some(rl)` use `route(&rl.method, &rl.path)` and `build_response`; on `None` build the 400. Finish with `stream.write_all(response.as_bytes())`.
