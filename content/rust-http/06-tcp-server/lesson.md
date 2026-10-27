---
title: A TCP Server
summary: Use TcpListener and TcpStream to accept real connections, read a request that arrives in pieces, and write a response back to the client.
minutes: 30
difficulty: impossible
tags: networking, tcp
id: rust-http/tcp-server
mode: harness
---
You now have a parser, a router and a response builder. They are all pure functions on strings. To become a server they need a **socket**: the operating system's handle for one end of a network connection.

## Listening and accepting

```rust
use std::net::TcpListener;

let listener = TcpListener::bind("127.0.0.1:8080")?;
for stream in listener.incoming() {
    let stream = stream?;
    // talk to this client
}
```

`bind` asks the OS to reserve an **address** (an IP plus a **port**) and start queueing incoming connections. `127.0.0.1` is the loopback address: only programs on your own machine can connect. Port `0` means "pick any free port", and `listener.local_addr()` tells you which one you got.

Each time a client connects, `accept` (or one step of `incoming()`) hands you a `TcpStream`: a connection to that one client. If you drop it, the connection closes.

## A stream is a Read and a Write

`TcpStream` implements the `Read` and `Write` traits, just like files do. Bring them into scope with `use std::io::{Read, Write};`.

```rust
let mut buf = [0u8; 1024];
let n = stream.read(&mut buf)?;   // n bytes were filled in
stream.write_all(b"hello")?;
```

## The big gotcha: read() returns whatever has arrived

TCP is a stream of bytes with no message boundaries. A single `read` may return half of the request line, or the headers but not the end. It returns `Ok(0)` only when the client closed its side (EOF). Code that does one `read` and assumes it has the whole request works on your laptop and breaks on a slow network.

So the rule is: **loop and accumulate** until you have seen what you are waiting for. For us, that is the blank line `\r\n\r\n` that ends the headers:

```text
read -> "GET /hel"            no blank line yet, keep going
read -> "lo HTTP/1.1\r\n\r\n"  now we have it
```

```rust
let mut buf: Vec<u8> = Vec::new();
let mut chunk = [0u8; 1024];
loop {
    let n = stream.read(&mut chunk)?;
    if n == 0 { break; }                       // client hung up
    buf.extend_from_slice(&chunk[..n]);
    if buf.windows(4).any(|w| w == b"\r\n\r\n") { break; }
    if buf.len() >= 8192 { break; }            // cap: never trust the client
}
```

Always cap the size. Otherwise a client can send bytes forever and eat all your memory. Note that `windows(4)` checks every 4-byte window of the buffer, so it finds the marker even when it is split across two reads.

## Writing the reply and closing

`write_all` keeps writing until every byte is sent (plain `write` may send only some). Because we always send `Connection: close`, the server closes the connection after replying by letting the stream drop at the end of the function. The client sees EOF, which is also how it knows the response is complete.

## Bad requests

If the first line does not parse, do not crash. Reply `400 Bad Request` with a short body. Never `unwrap` anything that came from the network.

## Your turn

The starter includes the [earlier lessons](/learn/rust-http/review-handler)' code. Write `handle_connection`: read the head, parse the request line, call `route`, build a response and write it. The tests start a real server on a random loopback port and connect to it.
