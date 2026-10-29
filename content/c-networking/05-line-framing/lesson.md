---
title: Framing a Line Protocol
summary: A stream has no message boundaries. Turn arbitrary chunks of bytes back into lines, the way SMTP, Redis-style commands and the start of HTTP do.
minutes: 25
difficulty: medium
tags: protocols, framing, parsing
id: c-networking/line-framing
mode: harness
---
A TCP connection is a river of bytes. If the client sends `PING\n` and `GET a\n`, the server may see `PING\nGE` then `T a\n`, or everything at once, or one byte at a time. To get messages back you must **frame** them: agree where one ends.

Text protocols such as SMTP, FTP, POP3 and the request line of HTTP use **lines**: a message ends with `\n`, or with `\r\n` (CRLF) on the Internet. The receiving code keeps a buffer:

1. **feed** whatever bytes arrived into the buffer;
2. repeatedly **pull out** one complete line, if there is one;
3. leave any incomplete tail in the buffer, waiting for more bytes.

```
buffer: "PING\r\nGET a\nPAR"
  -> line "PING"
  -> line "GET a"
  -> nothing more (the tail "PAR" has no newline yet)
```

Note step 2 is a *loop*: one `feed` can deliver several lines, so you pull until none are left.

## Limits

A hostile client can send gigabytes without a newline. Every frame parser needs a **maximum**; here the buffer holds 64 bytes. If a feed would overflow it, refuse (return an error) and let the caller drop the connection.

## Parsing a command

Once you have a line like `get   Name`, split it into a verb and an argument. Our commands are case-insensitive, so the verb is upper-cased, and spaces between verb and argument are skipped.

This lesson is pure memory: no sockets. The tests feed your buffer in awkward pieces, exactly the way a socket would.
