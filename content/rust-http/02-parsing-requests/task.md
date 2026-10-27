Define (in the starter) and implement:

```rust
struct Request {
    method: String,
    path: String,
    headers: HashMap<String, String>,
    body: String,
}

fn parse_request(raw: &str) -> Result<Request, String>
```

Rules:

- Split `raw` at the first `"\r\n\r\n"`. The part before is the head, the part after is the body (kept exactly as is). If there is no `"\r\n\r\n"`, the whole input is the head and the body is `""`.
- Split the head on `"\r\n"`. The first line is the request line. If the head is empty (so there is no request line), return `Err`. If the request line is not valid (use `parse_request_line`), return `Err`.
- Every other line is a header and must contain a `:`. The name is everything before the **first** colon, trimmed and lowercased. The value is everything after it, trimmed. A line without a colon returns `Err`.
- If a header name appears twice, the later one wins.
- The error message can be any text.
