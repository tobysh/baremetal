---
title: Channels in Depth
summary: Unbuffered versus buffered, close and the comma-ok receive, and ranging over a channel.
minutes: 25
difficulty: medium
tags: channels
id: go-concurrency/channels
mode: harness
---
A channel is a typed pipe between goroutines. Two properties decide how it behaves.

## Unbuffered and buffered

```go
a := make(chan int)    // unbuffered: send waits for a receiver
b := make(chan int, 3) // buffered: send only waits when 3 values are queued
```

An unbuffered send and receive **meet**: both goroutines wait until the other arrives, so it is also a synchronisation point. A buffered channel decouples them up to its capacity. `len(b)` is how many values are queued and `cap(b)` is the capacity. A buffer can let you fill a channel from the same goroutine that will read it later, with no deadlock, as long as you never exceed the capacity.

## Closing

`close(ch)` says "no more values will be sent". Only the **sender** should close, and closing twice (or sending on a closed channel) panics. Receivers can still drain what is left in the buffer, and after that every receive returns immediately with the zero value.

To tell a real zero from "closed", use the two-value receive:

```go
v, ok := <-ch // ok is false once ch is closed and empty
```

`for v := range ch` is that loop written for you: it ends when the channel is closed and empty. Forgetting to close means a `range` waits forever.

## Directions

`<-chan int` is receive-only and `chan<- int` is send-only. A bidirectional `chan int` converts to either automatically, which lets the compiler stop a function from misusing a channel it was only meant to read.
