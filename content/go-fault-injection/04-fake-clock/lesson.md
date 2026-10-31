---
title: Clocks That Jump
summary: Drive a token-bucket rate limiter with an explicit fake clock and survive time going backwards or leaping forward without minting or losing tokens.
minutes: 35
difficulty: hard
tags: time, fake-clock, rate-limiting
id: go-fault-injection/fake-clock
mode: harness
---
Code that reads the time directly cannot be tested: you would have to sleep, and you could never make time misbehave. Instead, pass the code a `Clock` and give the tests one they control:

```go
type Clock interface{ NowMs() int64 }

type fakeClock struct{ now int64 }
func (c *fakeClock) NowMs() int64   { return c.now }
func (c *fakeClock) Advance(d int64) { c.now += d }
func (c *fakeClock) Set(t int64)     { c.now = t }
```

`Set` can move time **backwards**, which wall clocks really do (NTP corrections, manual changes, VM restores). `Advance` with a huge number models a long suspend or a leap forward.

## A token bucket

A limiter holds up to `capacity` tokens and gains one token every `intervalMs`. `Allow()` spends a token if there is one. A naive version computes `elapsed = now - last` and adds `elapsed / interval`. Break it:

- **Backwards**: `elapsed` is negative, so tokens go negative, or `last` stays in the future and nothing refills for an hour.
- **Far forward**: `elapsed / interval` is enormous; adding it overflows or exceeds capacity.
- **Idle at capacity**: if `last` is left old while the bucket is full, a request just after shows a huge `elapsed`, which wrongly gives credit for time during which the bucket could not have gained anything.

## The exact rules

On every call, before anything else, run `refill(now)`:

1. If `now < last`: the clock went backwards. Set `last = now`. No tokens are added.
2. Else if the bucket is full: set `last = now`.
3. Else `n = (now - last) / intervalMs`. If `n >= capacity - tokens`, the bucket becomes full and `last = now`. Otherwise `tokens += n` and `last += n * intervalMs` (the leftover partial interval is kept).

The bucket starts full with `last` equal to the clock's reading at creation. `Allow` refills, then spends a token if any. `Tokens` refills and reports the count.

The hidden tests drive a random mix of advances, backward jumps and giant leaps, and compare every answer to a reference model that simulates refills one interval at a time.

## Limits of this model

The model has one clock and integer milliseconds. It rebases after a backward jump, which is one defensible policy; a monotonic clock avoids the problem entirely where one is available.
