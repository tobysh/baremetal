The starter defines:

```rust
#[derive(Debug, PartialEq)]
struct RequestLine {
    method: String,
    path: String,
    version: String,
}
```

Write `fn parse_request_line(line: &str) -> Option<RequestLine>`.

- The input is one line with no trailing `\r\n`.
- Split on single spaces. There must be **exactly 3** parts, otherwise return `None`.
- The third part (the version) must start with `"HTTP/"`, otherwise return `None`.
- Otherwise return `Some(RequestLine { method, path, version })`, copying the three parts as they appear. Do not change their case.
- An empty string, too few parts, too many parts (including doubled spaces) and a bad version are all `None`.
