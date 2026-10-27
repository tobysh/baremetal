---
title: What is HTTP?
summary: Learn how TCP and HTTP fit together, what an HTTP request looks like byte for byte, and parse the first line of one in Rust.
minutes: 20
difficulty: medium
tags: http, networking
id: rust-http/what-is-http
mode: harness
---
Your browser and a web server are two programs, usually on different machines. To talk, they need two things: a way to move bytes between them, and an agreement on what those bytes mean.

## Two layers

**TCP** (Transmission Control Protocol) is the first layer. It gives two programs a reliable, ordered pipe of bytes. You put bytes in one end, they come out the other end in the same order. TCP has no idea what the bytes mean, and it does not preserve message boundaries: ten bytes sent as one write might arrive as two reads of five.

**HTTP** (HyperText Transfer Protocol) is the second layer, built on top. It is an agreement about *what to put in the pipe*: the client sends a **request**, the server sends back a **response**.

```text
  client                         server
    |  ---- request  ------->      |
    |  <---- response -------      |
```

HTTP/1.1 is plain text. You can read it with your eyes.

## Anatomy of a request

```http
POST /notes HTTP/1.1
Host: example.com
Content-Length: 5

hello
```

On the wire, every line ends with two bytes, `\r\n` (carriage return, line feed), called **CRLF**. The raw text in Rust is:

```text
"POST /notes HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello"
```

A request has four parts:

1. **Request line**: method, path, version, separated by single spaces. The **method** says what to do (`GET` reads, `POST` sends data). The **path** says which resource. The **version** says which dialect of HTTP.
2. **Headers**: `Name: value` lines giving extra information.
3. **A blank line** (`\r\n\r\n` at the end of the last header). This is how the server knows the headers are over.
4. **Body**: optional bytes, such as a form or JSON. A `GET` usually has none.

The common mistake is splitting on `\n` only. The protocol says `\r\n`, and a stray `\r` left on the end of your version string will break comparisons that look correct.

## Parsing the request line

Real servers must handle bad input, because anyone on the network can send anything. So parsing returns an `Option`: `Some` when the line is valid, `None` when it is not.

```rust
let parts: Vec<&str> = "GET /index.html HTTP/1.1".split(' ').collect();
// ["GET", "/index.html", "HTTP/1.1"]
assert_eq!(parts.len(), 3);
```

`split(' ')` splits on every single space, so a line with a doubled space yields an empty piece and the wrong count, which is exactly what we want to reject. Once you have three parts, you check that the version starts with `"HTTP/"` and build the struct with owned `String`s, since the line you parsed will not live as long as the result.

## Your turn

The `RequestLine` struct is in the starter. Write `parse_request_line` so it returns `Some` for well-formed lines and `None` otherwise. The line you receive has no trailing `\r\n`.
