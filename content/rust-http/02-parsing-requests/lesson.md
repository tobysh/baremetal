---
title: Parsing Requests
summary: Split a raw HTTP request into request line, headers and body, handle case-insensitive header names, and report malformed input with Result.
minutes: 25
difficulty: hard
tags: http, parsing
id: rust-http/parsing-requests
mode: harness
---
In [the last lesson](/learn/rust-http/what-is-http) you parsed one line. Now we parse a whole request: everything a client sent, as one string.

## Head and body

A request is two sections separated by the first blank line, `\r\n\r\n`:

```text
GET /hello HTTP/1.1\r\n
Host: localhost\r\n
Accept: text/plain\r\n
\r\n
...body, if any...
```

Everything before the separator is the **head** (request line plus headers). Everything after is the **body**. `str::split_once` is made for this:

```rust
let raw = "POST /x HTTP/1.1\r\nHost: a\r\n\r\nhi";
let (head, body) = raw.split_once("\r\n\r\n").unwrap();
assert_eq!(head, "POST /x HTTP/1.1\r\nHost: a");
assert_eq!(body, "hi");
```

If there is no separator at all, the client has not finished sending the head. For this lesson, treat the whole input as the head with an empty body.

## Headers

Inside the head, split on `"\r\n"`. The first line is the request line. Each remaining line is a header: a name, a colon, a value.

```text
Content-Length: 5
content-length:5
CONTENT-LENGTH:   5
```

These are all the same header. The HTTP spec says header **names are case-insensitive**, and whitespace around the value is not part of the value. So we normalise: lowercase the name, trim the value, and store them in a `HashMap<String, String>`. After that, lookups are simply `headers.get("content-length")`.

```rust
let line = "Content-Length:   5";
let (name, value) = line.split_once(':').unwrap();
let name = name.trim().to_lowercase();
let value = value.trim();
```

Use `split_once(':')` and not `split(':')`. A header like `Host: localhost:8080` contains two colons, and only the first one separates name from value. Splitting on every colon is the classic bug here.

## Errors are normal

Input from the network is untrusted. A line with no colon is not a header, and an empty request has no request line. Do not `unwrap` or panic, because one bad client would crash your server. Return `Result<Request, String>` with a message describing the problem, and let the caller decide what to do (in a later lesson, answer `400 Bad Request`).

## About Content-Length

`Content-Length` tells the receiver how many bytes of body to expect, which matters when reading from a socket in pieces. We will not enforce it yet: the body here is simply whatever follows the blank line.

## Your turn

The starter includes `parse_request_line` from [the previous lesson](/learn/rust-http/what-is-http). Implement `parse_request`.
