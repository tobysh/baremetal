Write two functions.

`fn reason_phrase(status: u16) -> &'static str`

| status | phrase |
|---|---|
| 200 | `OK` |
| 201 | `Created` |
| 400 | `Bad Request` |
| 404 | `Not Found` |
| 405 | `Method Not Allowed` |
| 500 | `Internal Server Error` |
| anything else | `Unknown` |

`fn build_response(status: u16, content_type: &str, body: &str) -> String`

Return exactly this text, with the placeholders filled in:

```text
HTTP/1.1 {status} {reason}\r\nContent-Type: {content_type}\r\nContent-Length: {bytes}\r\nConnection: close\r\n\r\n{body}
```

- The header order is fixed: `Content-Type`, then `Content-Length`, then `Connection`.
- `{bytes}` is the length of `body` in **bytes** (`"héllo"` is 6).
- Every line ends in `\r\n`, and an empty body still gets the blank line and `Content-Length: 0`.
- `{reason}` comes from `reason_phrase`.
