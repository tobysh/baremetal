---
title: Tickers and Rate Limiting
summary: Do something on a steady beat with time.Ticker, pace a stream of values, and poll until a condition holds.
minutes: 30
difficulty: hard
tags: time, ticker, rate-limit
id: go-concurrency/tickers-and-rate-limits
mode: harness
---
`time.After` fires once. For something that repeats, use a **ticker**: a channel that receives a value every interval.

```go
t := time.NewTicker(200 * time.Millisecond)
defer t.Stop() // always stop it, or it keeps running

for {
	select {
	case <-t.C:
		doPeriodicWork()
	case <-ctx.Done():
		return
	}
}
```

Points to remember:

- `Stop` the ticker when you are done (with `defer`). Since Go 1.23 an unreferenced ticker is garbage-collected, but stopping is still good manners and required for older code.
- A ticker has a one-slot buffer and **drops** ticks if you are slow, rather than queueing them. It is a beat, not a counter.
- Do not rely on exact timing. The operating system may wake you late. Test and design with margins.

## Rate limiting

To let at most one item through per interval, wait for a tick before forwarding each:

```go
for v := range in {
	<-t.C
	out <- v
}
```

(A production limiter often allows short bursts using a token bucket, but a ticker is the simplest pacing tool.)

## Polling

"Check every so often until it is ready, or give up" combines a ticker with a context:

```go
select {
case <-t.C:
	if ready() { return true }
case <-ctx.Done():
	return false
}
```

Prefer a signal (a closed channel, a condition variable) over polling when you control both sides. Poll when you do not, such as waiting for a file or a remote service.
