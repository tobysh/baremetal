---
title: Fakes and Fake Clocks
summary: Make time-dependent code testable by injecting a Clock interface and driving a fake one by hand, with no sleeping.
minutes: 40
difficulty: hard
tags: testing, interfaces, time
id: go-services/fakes-and-clocks
mode: harness
---
Code that calls `time.Now()` directly is hard to test. To check that a cache entry expires after a minute, the test would have to wait a minute, or sleep a "short" while and hope the machine is not busy. Both are slow or flaky.

The fix is **dependency injection** with a small interface:

```go
type Clock interface {
	Now() time.Time
}
```
Production code gets a real clock; tests get a fake one that only moves when the test says so:

```go
type FakeClock struct {
	mu  sync.Mutex
	now time.Time
}

func (c *FakeClock) Now() time.Time { c.mu.Lock(); defer c.mu.Unlock(); return c.now }
func (c *FakeClock) Advance(d time.Duration) { c.mu.Lock(); c.now = c.now.Add(d); c.mu.Unlock() }
```
Now "wait a minute" is `clock.Advance(time.Minute)`: instant and exact.

## Test doubles

Fakes, stubs and spies are all *test doubles*: simple stand-ins for a real dependency. The key design rule is to depend on the **smallest interface you need** (`Now()` here), defined where it is *used*, not where it is implemented. That is why Go interfaces are usually one or two methods.

Boundaries matter in a TTL cache. If an entry lives for 10 seconds, is it still there at *exactly* 10 seconds? Decide, document it, and test both sides of the line.
