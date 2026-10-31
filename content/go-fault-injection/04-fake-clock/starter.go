package main

import "fmt"

type Clock interface{ NowMs() int64 }

type Limiter struct {
	clock    Clock
	capacity int
	interval int64
	tokens   int
	last     int64
}

func NewLimiter(c Clock, capacity int, intervalMs int64) *Limiter {
	return &Limiter{clock: c, capacity: capacity, interval: intervalMs, tokens: capacity, last: c.NowMs()}
}

// refill is naive: it trusts the clock to be monotonic and small steps.
func (l *Limiter) refill() {
	now := l.clock.NowMs()
	l.tokens += int((now - l.last) / l.interval)
	l.last = now
}

func (l *Limiter) Allow() bool {
	l.refill()
	if l.tokens > 0 {
		l.tokens--
		return true
	}
	return false
}

func (l *Limiter) Tokens() int {
	l.refill()
	return l.tokens
}

type stepClock struct{ now int64 }

func (c *stepClock) NowMs() int64 { return c.now }

func main() {
	c := &stepClock{}
	l := NewLimiter(c, 2, 1000)
	fmt.Println(l.Allow(), l.Allow(), l.Allow())
	c.now = 1000
	fmt.Println(l.Allow(), l.Allow())
}
