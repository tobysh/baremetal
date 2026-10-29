---
title: Select, Default and Timeouts
summary: Wait on several channels at once, make a send or receive non-blocking, and give up after a deadline.
minutes: 30
difficulty: medium
tags: select, channels, time
id: go-concurrency/select-and-timeouts
mode: harness
---
`select` is a `switch` for channel operations. It waits until **one** of its cases can proceed, runs it, and moves on. If several are ready at once, Go picks one at random.

```go
select {
case v := <-a:
	fmt.Println("a said", v)
case b <- 42:
	fmt.Println("sent to b")
}
```

## Non-blocking with default

A `default` case runs immediately if no other case is ready, so the whole `select` never waits:

```go
select {
case ch <- v:
	return true // there was room
default:
	return false // full, drop it
}
```

This is how you "try" an operation: drop work when a queue is full, or poll for a value without stalling.

## Timeouts

`time.After(d)` returns a channel that receives one value after `d`. Put it in a `select` for a timeout:

```go
select {
case v := <-ch:
	use(v)
case <-time.After(2 * time.Second):
	return errTimeout
}
```

Two cautions. A **nil channel** never becomes ready, so a case on a nil channel is effectively switched off (useful for disabling a branch). And `time.After` allocates a timer; in a hot loop prefer a reusable `time.NewTimer` and `Stop` it. You will see `context` do timeouts better in a later lesson.
