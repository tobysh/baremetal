---
title: Building Responses
summary: Write HTTP responses by hand: the status line, reason phrases, headers, and why Content-Length counts bytes rather than characters.
minutes: 20
difficulty: medium
id: rust-http/building-responses
mode: harness
---
The server's half of the conversation is the **response**. It has the same shape as a request: a first line, headers, a blank line, a body.

```http
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 5
Connection: close

hello
```

In real bytes, each line ends with `\r\n` and there is one more `\r\n` between the headers and the body.

## The status line

```text
HTTP/1.1 404 Not Found
 version  code reason
```

The **status code** is the number programs care about. Codes are grouped by their first digit: `2xx` success, `4xx` the client made a mistake, `5xx` the server failed. The **reason phrase** is human-readable text. Clients ignore it, but you must still send it.

Mapping a number to text is a job for `match`:

```rust
fn reason_phrase(status: u16) -> &'static str {
    match status {
        200 => "OK",
        // ...
        _ => "Unknown",
    }
}
```

The return type is `&'static str` because the phrases are string literals baked into the program, which live for the whole run.

## Headers that matter

- `Content-Type` tells the client how to interpret the body (`text/plain`, `text/html`, `application/json`).
- `Content-Length` tells the client exactly how many bytes of body follow. Without it, the client does not know where the body ends.
- `Connection: close` says we will close the TCP connection after this response. That makes it simple: one request, one response, done.

## Bytes, not characters

This is where the common mistake lives. `Content-Length` counts **bytes**. In Rust, `str::len()` returns bytes too, but `chars().count()` returns characters, and for non-ASCII text they differ:

```rust
let s = "héllo";
assert_eq!(s.len(), 6);           // the 'é' takes 2 bytes in UTF-8
assert_eq!(s.chars().count(), 5);
```

If you announce 5 and send 6, the client cuts the last byte off, or worse, waits for data that never comes. Always use `.len()` on the `&str` that you are going to send.

## Building the string

`format!` makes this easy. Remember that `\r\n` is two characters in a Rust string literal:

```rust
let head = format!("HTTP/1.1 {} {}\r\n", 200, "OK");
```

Header order is not significant in HTTP, but fixed order makes your output testable. In this lesson the order is exactly: `Content-Type`, `Content-Length`, `Connection`.

## Your turn

Implement `reason_phrase` and `build_response`. The task page has the exact format.
