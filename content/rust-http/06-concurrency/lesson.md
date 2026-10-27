---
title: Concurrency
summary: See why a single-threaded server stalls on one slow client, then fix it with one thread per connection using std::thread::spawn and move closures.
minutes: 25
difficulty: impossible
id: rust-http/concurrency
mode: harness
---
Your server from the last lesson handles one client at a time. For a quick test that is fine. In the real world it has a serious flaw.

## The slow client problem

```rust
for stream in listener.incoming() {
    handle_connection(stream.unwrap());   // blocks until this client is done
}
```

`read` **blocks**: the thread sleeps until data arrives. If client A connects and then sits there sending nothing (a slow phone, a stalled network, or an attacker), the loop is stuck inside `handle_connection` for A. Client B connects, and the OS happily queues it, but nobody calls `accept` for it until A is finished.

```text
time ->
server:  [ waiting on A ............................ ][ B ]
client B:   connect ......... waiting ................. reply
```

One slow client delays everybody. The common mistake is testing only with fast clients and never noticing.

## One thread per connection

A **thread** is an independent line of execution inside your program. While one thread is blocked in `read`, others keep running. So: accept a connection, hand it to a new thread, and immediately go back to accepting.

```rust
use std::thread;

let handle = thread::spawn(move || {
    handle_connection(stream);
});
```

`thread::spawn` takes a closure. The `move` keyword makes the closure take **ownership** of the variables it uses, here `stream`. This is required: the new thread may outlive the loop iteration that created it, so it cannot merely borrow. Because `TcpStream` is moved, the compiler guarantees no two threads fight over one connection.

`spawn` returns a `JoinHandle`. Calling `handle.join()` waits for that thread to finish. Keep the handles in a `Vec` so you can wait for all of them:

```rust
let mut handles = Vec::new();
for _ in 0..n {
    let (stream, _addr) = listener.accept().unwrap();
    handles.push(thread::spawn(move || handle_connection(stream)));
}
for h in handles {
    h.join().unwrap();
}
```

`accept()` gives you the next connection and the peer's address. Here we use it directly instead of `incoming()`, so we can stop after `n` connections.

## Why this is not the end of the story

Thread per connection is simple and correct, but each thread costs memory and scheduling time. A busy server with 100,000 connections cannot have 100,000 threads. Two common next steps:

- A **thread pool**: a fixed number of worker threads that take connections from a shared queue. This caps resource use.
- **Async I/O**: a small number of threads multiplexing many connections, where waiting for data does not block a thread. In Rust this is the `async`/`await` world (libraries such as Tokio).

Both build on exactly what you have learned: sockets, parsing, routing, responses.

## Your turn

`handle_connection` from last lesson is in the starter (with a 5 second read timeout so dead clients do not hang a thread forever). The starter's `serve` is sequential. Rewrite it so each connection runs on its own thread. The test makes one client stall while another needs a quick answer.
